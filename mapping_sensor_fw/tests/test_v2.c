#include "app.h"
#include "scan_controller.h"
#include "sensor_config.h"
#include "frame_buffer.h"
#include "signal_processing.h"
#include "detection.h"
#include "command_parser.h"
#include "command_dispatcher.h"
#include "response_formatter.h"
#include "diagnostics.h"
#include "mock_hardware.h"
#include <string.h>

volatile uint32_t test_checks, test_fail_line, test_cases;
#define CHECK(condition) do { ++test_checks; if (!(condition)) { test_fail_line = __LINE__; return false; } } while (0)

static AppStatus Status(void)
{
    AppStatus status;
    App_GetStatus(&status);
    return status;
}

static void Pump(void)
{
    for (unsigned i = 0; i < 64; ++i) { App_Process(); }
}

static void Send(const char *command)
{
    Mock_QueueInput(command);
    Mock_QueueInput("\r\n");
    Pump();
}

static bool CompleteChannel(unsigned channel, uint16_t off, uint16_t on)
{
    CHECK(Status().state == SCAN_SELECT_WAIT);
    CHECK(Mock_Channel() == channel && !Mock_Tx() && Mock_Rx());
    Mock_CompleteWait(); App_Process();
    CHECK(Status().state == SCAN_OFF_ACQUIRE && Mock_Sampling());
    Mock_CompleteSamples(off); App_Process();
    CHECK(Status().state == SCAN_LIGHT_WAIT && Mock_Tx());
    Mock_CompleteWait(); App_Process();
    CHECK(Status().state == SCAN_ON_ACQUIRE && Mock_Sampling());
    Mock_CompleteSamples(on); App_Process();
    CHECK(Mock_Violations() == 0);
    return true;
}

static bool CompleteFrame(uint16_t off, uint16_t on)
{
    for (unsigned channel = 0; channel < SENSOR_CHANNELS; ++channel) {
        CHECK(CompleteChannel(channel, off, on));
    }
    return true;
}

static bool TestSingle(void)
{
    App_Init();
    CHECK(Status().state == SCAN_IDLE && !Mock_Tx() && !Mock_Rx());
    CHECK(strstr(Mock_Output(), "FW 0.2 READY") != NULL);
    Send("SINGLE");
    CHECK(strstr(Mock_Output(), "OK\r\n") != NULL);
    CHECK(Scan_StartSingle() == FW_INVALID_STATE);
    CHECK(CompleteFrame(100, 900));
    SensorFrame frame;
    CHECK(App_GetLatestFrame(&frame));
    CHECK(frame.sequence == 1 && frame.valid == SENSOR_ALL && frame.signal[19] == 800);
    CHECK(frame.detected == 0 && Status().state == SCAN_IDLE && !Mock_Tx() && Mock_Dac() == 0);
    Mock_ClearOutput(); Send("RESULT");
    CHECK(strstr(Mock_Output(), "FRAME=1 CFG=1 VALID=FFFFF DETECT=00000 SAT=00000") != NULL);
    CHECK(strstr(Mock_Output(), "CH=19 OFF=100 ON=900 SIG=800\r\n") != NULL);
    return true;
}

static bool TestHysteresis(void)
{
    App_Init(); CHECK(Scan_StartContinuous() == FW_OK);
    CHECK(CompleteFrame(100, 900));
    CHECK(CompleteFrame(100, 900));
    SensorFrame frame; CHECK(App_GetLatestFrame(&frame)); CHECK(frame.detected == SENSOR_ALL);
    CHECK(CompleteFrame(100, 550)); CHECK(App_GetLatestFrame(&frame)); CHECK(frame.detected == SENSOR_ALL);
    CHECK(CompleteFrame(100, 450)); CHECK(App_GetLatestFrame(&frame)); CHECK(frame.detected == SENSOR_ALL);
    CHECK(CompleteFrame(100, 450)); CHECK(App_GetLatestFrame(&frame)); CHECK(frame.detected == 0);
    Scan_EmergencyStop(); CHECK(!Mock_Tx());
    return true;
}

static bool TestConfigBoundary(void)
{
    App_Init(); CHECK(Scan_StartContinuous() == FW_OK);
    SensorConfig config; uint32_t version;
    Config_GetSnapshot(&config, &version); config.dac[0] = 1000;
    CHECK(Config_Stage(&config) == FW_OK);
    CHECK(Config_GetVersion() == 1);
    CHECK(CompleteFrame(100, 900));
    SensorFrame frame; CHECK(App_GetLatestFrame(&frame));
    CHECK(frame.config_version == 1 && Config_GetVersion() == 2 && Mock_Dac() == 1000);
    CHECK(CompleteFrame(100, 900)); CHECK(App_GetLatestFrame(&frame));
    CHECK(frame.config_version == 2 && frame.detected == 0);
    return true;
}

static bool TestConfigValidation(void)
{
    App_Init(); SensorConfig config;
    Config_LoadDefaults(&config); CHECK(Config_Validate(&config));
    config.dac[19] = 4096; CHECK(Config_Stage(&config) == FW_INVALID_ARGUMENT);
    Config_LoadDefaults(&config); config.confirm = 0; CHECK(!Config_Validate(&config));
    Config_LoadDefaults(&config); config.sample_us = 1; CHECK(!Config_Validate(&config));
    Config_LoadDefaults(&config); config.gain_q12[0] = 0; CHECK(!Config_Validate(&config));
    CHECK(!Config_Validate(NULL)); CHECK(Config_GetVersion() == 1);
    return true;
}

static bool TestStops(void)
{
    for (unsigned phase = 0; phase < 4; ++phase) {
        App_Init(); CHECK(Scan_StartContinuous() == FW_OK);
        if (phase >= 1) { Mock_CompleteWait(); App_Process(); }
        if (phase >= 2) { Mock_CompleteSamples(100); App_Process(); }
        if (phase >= 3) { Mock_CompleteWait(); App_Process(); }
        Send("ESTOP");
        CHECK(Status().state == SCAN_IDLE && !Mock_Tx() && !Mock_Rx());
        CHECK(!FrameBuffer_IsBusy() && !Mock_Waiting() && !Mock_Sampling());
    }
    App_Init(); CHECK(Scan_StartContinuous() == FW_OK); Scan_RequestStop();
    CHECK(CompleteChannel(0, 100, 900));
    CHECK(Status().state == SCAN_IDLE && Status().frames == 0 && !FrameBuffer_IsBusy());
    return true;
}

static bool TestTimeoutAndClear(void)
{
    App_Init(); CHECK(Scan_StartContinuous() == FW_OK);
    Mock_CompleteWait(); App_Process(); Mock_CompleteSamples(100); App_Process();
    CHECK(Mock_Tx()); Mock_Timeout(); App_Process();
    CHECK(Status().state == SCAN_FAULT && Status().fault == 1 && !Mock_Tx());
    CHECK(!FrameBuffer_IsBusy() && !Mock_Sampling());
    CHECK(Scan_StartSingle() == FW_INVALID_STATE);
    Send("CLEAR"); CHECK(Status().state == SCAN_IDLE && Status().fault == 0);
    Mock_ClearOutput(); Send("CLEAR"); CHECK(strcmp(Mock_Output(), "ERR\r\n") == 0);
    CHECK(Scan_StartSingle() == FW_OK); CHECK(CompleteFrame(100, 900));
    return true;
}

static bool TestSaturationAndPolarity(void)
{
    App_Init(); CHECK(Scan_StartSingle() == FW_OK); CHECK(CompleteFrame(100, 4095));
    SensorFrame frame; CHECK(App_GetLatestFrame(&frame));
    CHECK(frame.valid == 0 && frame.saturated == SENSOR_ALL && frame.detected == 0);
    App_Init(); SensorConfig config; Config_LoadDefaults(&config);
    config.polarity = 0; config.confirm = 1; config.offset[0] = 100; config.gain_q12[0] = 8192;
    CHECK(Config_Stage(&config) == FW_OK); CHECK(Scan_StartSingle() == FW_OK);
    CHECK(CompleteFrame(900, 100)); CHECK(App_GetLatestFrame(&frame));
    CHECK(frame.signal[0] == 1400 && frame.detected == SENSOR_ALL);
    return true;
}

static bool TestBadSamplesAndEvents(void)
{
    App_Init(); Scan_OnWaitComplete(); CHECK(Status().fault == 2);
    App_Init(); uint16_t data[4] = {0}; Scan_OnSamples(data, 4); CHECK(Status().fault == 3);
    App_Init(); CHECK(Scan_StartSingle() == FW_OK); Mock_CompleteWait(); App_Process();
    Mock_CompleteSamples(5000); App_Process(); CHECK(Status().fault == 4 && !Mock_Tx());
    App_Init(); CHECK(Scan_StartSingle() == FW_OK); Mock_CompleteWait(); App_Process();
    Scan_OnSamples(data, 3); CHECK(Status().fault == 4);
    return true;
}

static bool TestBufferOwnership(void)
{
    FrameBuffer_Init(); unsigned first, second, third; SensorConfig config;
    Config_LoadDefaults(&config);
    CHECK(FrameBuffer_AcquireWrite(&first) == FW_OK);
    CHECK(FrameBuffer_Begin(first, &config, 7) == FW_OK);
    CHECK(FrameBuffer_Publish(first, 1) == FW_INVALID_STATE);
    for (unsigned i = 0; i < SENSOR_CHANNELS; ++i) {
        CHECK(FrameBuffer_CommitChannel(first, i, false, 100, false) == FW_OK);
        CHECK(FrameBuffer_CommitChannel(first, i, true, 900, false) == FW_OK);
    }
    CHECK(FrameBuffer_CommitChannel(first, 0, true, 900, false) == FW_INVALID_STATE);
    CHECK(FrameBuffer_Publish(first, 1) == FW_OK);
    CHECK(FrameBuffer_AcquireWrite(&second) == FW_OK && second != first);
    CHECK(FrameBuffer_AcquireWrite(&third) == FW_BUSY);
    CHECK(FrameBuffer_Release(first) == FW_INVALID_STATE);
    const RawFrame *raw = FrameBuffer_GetReady(&third);
    CHECK(raw != NULL && third == first && raw->config.dac[0] == 512 && raw->values.config_version == 7);
    CHECK(FrameBuffer_Discard(first) == FW_INVALID_STATE);
    CHECK(FrameBuffer_Release(first) == FW_OK);
    CHECK(FrameBuffer_Release(first) == FW_INVALID_STATE);
    CHECK(FrameBuffer_Discard(second) == FW_OK && !FrameBuffer_IsBusy());
    return true;
}

static bool TestSignalBoundaries(void)
{
    uint16_t data[4] = {0, 1, 4094, 4095}, mean;
    CHECK(Signal_CalculateMean(data, 4, &mean) == FW_OK && mean == 2047);
    CHECK(Signal_CheckSaturation(data, 4));
    CHECK(Signal_CalculateMean(data, 0, &mean) == FW_INVALID_ARGUMENT);
    CHECK(Signal_CalculateMean(NULL, 4, &mean) == FW_INVALID_ARGUMENT);
    CHECK(Signal_CalculateDifference(4095, 0, 1) == -4095);
    CHECK(Signal_ApplyCalibration(4095, -32768, 16384) == 147452);
    return true;
}

static bool TestParser(void)
{
    Command command;
    const char *valid[] = {"STATUS", "RESULT", "SINGLE", "START", "STOP", "ESTOP", "CLEAR", "HELP", "SAVE", "DAC 19 4095", "THRESH 0 500 400", "POLARITY 0"};
    const char *invalid[] = {"", "status", "START x", "DAC -1 1", "DAC 20 1", "DAC 0 4096", "DAC 0 999999999999999999999999", "DAC 0 1x", "DAC 0", "THRESH 0 400 500", "THRESH 0 400 400", "POLARITY 2"};
    for (unsigned i = 0; i < sizeof(valid) / sizeof(valid[0]); ++i) {
        CHECK(CommandParser_Parse(valid[i], &command) == FW_OK);
    }
    for (unsigned i = 0; i < sizeof(invalid) / sizeof(invalid[0]); ++i) {
        CHECK(CommandParser_Parse(invalid[i], &command) == FW_INVALID_ARGUMENT);
    }
    CHECK(CommandParser_Parse("  DAC 0 0001  ", &command) == FW_OK && command.args[1] == 1);
    return true;
}

static bool TestLineRecovery(void)
{
    App_Init(); Mock_ClearOutput();
    char oversized[150]; memset(oversized, 'A', sizeof(oversized) - 1); oversized[149] = 0;
    Send(oversized); CHECK(strstr(Mock_Output(), "ERR\r\n") != NULL);
    Mock_ClearOutput(); Send("STATUS"); CHECK(strstr(Mock_Output(), "STATE=0 FAULT=0") != NULL);
    Mock_ClearOutput(); Mock_QueueInput("STA"); App_Process(); Mock_SetRxOverrun();
    Send("TUS"); CHECK(strcmp(Mock_Output(), "ERR\r\n") == 0);
    Mock_ClearOutput(); Send("STATUS"); CHECK(strstr(Mock_Output(), "RX_DROP=1") != NULL);
    return true;
}

static bool TestResponseSnapshot(void)
{
    App_Init(); Mock_ClearOutput();
    CommandReply reply = {0}; reply.type = REPLY_FRAME; reply.frame.sequence = 23;
    for (unsigned i = 0; i < SENSOR_CHANNELS; ++i) { reply.frame.on[i] = 900; }
    Mock_BlockOutput(true); CHECK(ResponseFormatter_Begin(&reply) == FW_OK);
    reply.frame.sequence = 99; reply.frame.on[19] = 1;
    CHECK(ResponseFormatter_Begin(&reply) == FW_BUSY);
    Pump(); CHECK(strlen(Mock_Output()) == 0 && ResponseFormatter_IsBusy());
    ResponseFormatter_QueueAck(false); Mock_BlockOutput(false); Pump();
    CHECK(strstr(Mock_Output(), "FRAME=23") != NULL && strstr(Mock_Output(), "FRAME=99") == NULL);
    CHECK(strstr(Mock_Output(), "CH=19 OFF=0 ON=900 SIG=0\r\nERR\r\n") != NULL);
    CHECK(!ResponseFormatter_IsBusy());
    return true;
}

static bool TestStopWhileResponding(void)
{
    App_Init(); CHECK(Scan_StartContinuous() == FW_OK);
    CommandReply reply = {0}; reply.type = REPLY_FRAME;
    CHECK(ResponseFormatter_Begin(&reply) == FW_OK); Mock_BlockOutput(true);
    Send("ESTOP"); CHECK(Status().state == SCAN_IDLE && !Mock_Tx());
    Mock_BlockOutput(false); Pump(); CHECK(strstr(Mock_Output(), "OK\r\n") != NULL);
    return true;
}

static bool TestRepeated(void)
{
    App_Init(); CHECK(Scan_StartContinuous() == FW_OK);
    for (unsigned i = 0; i < 100; ++i) {
        CHECK(CompleteFrame(100, 900)); CHECK(Status().frames == i + 1u);
    }
    Send("STOP"); CHECK(CompleteChannel(0, 100, 900));
    CHECK(Status().state == SCAN_IDLE && Status().frames == 100 && !Mock_Tx());
    return true;
}

uint32_t tests_run(void)
{
    test_checks = test_fail_line = test_cases = 0;
    bool (*cases[])(void) = {
        TestSingle, TestHysteresis, TestConfigBoundary, TestConfigValidation,
        TestStops, TestTimeoutAndClear, TestSaturationAndPolarity, TestBadSamplesAndEvents,
        TestBufferOwnership, TestSignalBoundaries, TestParser, TestLineRecovery,
        TestResponseSnapshot, TestStopWhileResponding, TestRepeated
    };
    for (unsigned i = 0; i < sizeof(cases) / sizeof(cases[0]); ++i) {
        if (!cases[i]()) { return i + 1u; }
        ++test_cases;
    }
    return 0;
}
