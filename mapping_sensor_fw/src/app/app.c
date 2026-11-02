#include "app.h"
#include "board.h"
#include "channel_selector.h"
#include "light_control.h"
#include "adc_acquisition.h"
#include "measurement_timer.h"
#include "uart_transport.h"
#include "watchdog_service.h"
#include "scan_controller.h"
#include "frame_buffer.h"
#include "sensor_config.h"
#include "signal_processing.h"
#include "detection.h"
#include "diagnostics.h"
#include "command_parser.h"
#include "command_dispatcher.h"
#include "response_formatter.h"

static SensorFrame latest_frame;
static bool has_latest;
static uint32_t previous_rx_overruns;

static void CancelMeasurement(void)
{
    MeasurementTimer_CancelWait();
    Acquisition_Cancel();
}

void App_Init(void)
{
    Board_Init();
    LightControl_Init();
    ChannelSelector_Init();
    Config_Init();
    FrameBuffer_Init();
    Detection_Reset();
    Diagnostics_Init();
    MeasurementTimer_Init();
    Acquisition_Init();
    UartTransport_Init();
    CommandParser_Init();
    ResponseFormatter_Init(UartTransport_TryWrite);
    const ScanInterfaces interfaces = {
        LightControl_DisableTx, ChannelSelector_DisableRx, ChannelSelector_Select,
        LightControl_SetLevel, ChannelSelector_EnableRx, LightControl_EnableTx,
        MeasurementTimer_StartWait, Acquisition_Start, CancelMeasurement
    };
    Scan_Init(&interfaces);
    WatchdogService_Init();
    previous_rx_overruns = 0;
    has_latest = false;
    (void)UartTransport_TryWrite("MAPPING20 FW 0.2 READY / IDLE / RAM CONFIG\r\n");
}

void App_GetStatus(AppStatus *status)
{
    Scan_GetStatus(status);
    UartTransport_GetCounters(&status->rx_dropped, &status->tx_dropped);
    status->tx_dropped += ResponseFormatter_GetDrops();
}

bool App_GetLatestFrame(SensorFrame *frame)
{
    if (!has_latest) {
        return false;
    }
    *frame = latest_frame;
    return true;
}

static void ProcessMeasurementEvents(void)
{
    uint32_t key = Board_EnterCritical();
    if (MeasurementTimer_TakeTimeout()) {
        Scan_EnterFault(1);
    }
    if (MeasurementTimer_TakeWaitComplete()) {
        Scan_OnWaitComplete();
    }
    uint32_t generation;
    const uint16_t *samples = Acquisition_GetSamples(&generation);
    if (samples != NULL) {
        (void)generation;
        Acquisition_Release();
        Scan_OnSamples(samples, SENSOR_SAMPLES);
    }
    Board_ExitCritical(key);
}

static void ProcessReadyFrame(void)
{
    unsigned handle;
    const RawFrame *raw = FrameBuffer_GetReady(&handle);
    if (raw == NULL) {
        return;
    }
    SensorFrame processed;
    Signal_ProcessFrame(raw, &processed);
    Detection_UpdateFrame(&processed, &raw->config);
    latest_frame = processed;
    has_latest = true;
    (void)FrameBuffer_Release(handle);
}

static void ExecuteCommand(const Command *command)
{
    bool busy = ResponseFormatter_IsBusy();
    bool stop = command->type == CMD_STOP || command->type == CMD_ESTOP;
    if (busy && !stop) {
        ResponseFormatter_QueueAck(false);
        return;
    }
    AppStatus status;
    CommandReply reply;
    App_GetStatus(&status);
    uint32_t key = Board_EnterCritical();
    CommandDispatcher_Execute(command, &status, has_latest ? &latest_frame : NULL, &reply);
    Board_ExitCritical(key);
    if (busy) {
        ResponseFormatter_QueueAck(reply.ok);
    } else {
        (void)ResponseFormatter_Begin(&reply);
    }
}

static void ProcessCommands(void)
{
    uint32_t rx_overruns, tx_dropped;
    UartTransport_GetCounters(&rx_overruns, &tx_dropped);
    if (rx_overruns != previous_rx_overruns) {
        previous_rx_overruns = rx_overruns;
        CommandParser_InvalidateLine();
    }
    char byte;
    Command command;
    unsigned budget = 16;
    while (budget-- != 0 && UartTransport_ReadByte(&byte)) {
        ParserResult result = CommandParser_FeedByte(byte, &command);
        if (result == PARSER_READY) {
            ExecuteCommand(&command);
        } else if (result == PARSER_ERROR) {
            ResponseFormatter_QueueAck(false);
        }
    }
}

void App_Process(void)
{
    ProcessMeasurementEvents();
    ProcessReadyFrame();
    ProcessCommands();
    uint32_t key = Board_EnterCritical();
    Scan_Process();
    Board_ExitCritical(key);
    ResponseFormatter_Process();
    UartTransport_ProcessTx();
    AppStatus status;
    App_GetStatus(&status);
    Board_SetFaultIndicator(status.state == SCAN_FAULT);
    WatchdogService_Process();
}
