#include "scan_controller.h"
#include "sensor_config.h"
#include "frame_buffer.h"
#include "signal_processing.h"
#include "diagnostics.h"
#include "detection.h"

static ScanInterfaces io;
static ScanState state;
static SensorConfig frame_config;
static uint8_t channel;
static bool continuous, stop_requested, writing;
static unsigned write_handle;
static uint32_t sequence;

static void Scan_MakeSafe(void)
{
    io.tx_off();
    io.cancel();
    io.rx_off();
    (void)io.set_level(0);
}

static void Scan_AbortFrame(void)
{
    Scan_MakeSafe();
    if (writing) {
        (void)FrameBuffer_Discard(write_handle);
        writing = false;
    }
}

void Scan_EnterFault(uint32_t code)
{
    Scan_AbortFrame();
    Diagnostics_ReportFault(code, ((uint32_t)state << 8) | channel);
    state = SCAN_FAULT;
    continuous = false;
}

static void Scan_PrepareChannel(void)
{
    io.tx_off();
    io.rx_off();
    if (io.select(channel, channel) != FW_OK || io.set_level(frame_config.dac[channel]) != FW_OK) {
        Scan_EnterFault(3);
        return;
    }
    io.rx_on();
    state = SCAN_SELECT_WAIT;
    if (io.wait(frame_config.select_us) != FW_OK) {
        Scan_EnterFault(2);
    }
}

static FwStatus Scan_PrepareFrame(void)
{
    if (FrameBuffer_IsBusy()) {
        return FW_BUSY;
    }
    (void)Config_ApplyAtFrameBoundary();
    uint32_t version;
    Config_GetSnapshot(&frame_config, &version);
    if (FrameBuffer_AcquireWrite(&write_handle) != FW_OK) {
        return FW_BUSY;
    }
    writing = true;
    if (FrameBuffer_Begin(write_handle, &frame_config, version) != FW_OK) {
        Scan_EnterFault(3);
        return FW_HARDWARE_ERROR;
    }
    channel = 0;
    Scan_PrepareChannel();
    return state == SCAN_FAULT ? FW_HARDWARE_ERROR : FW_OK;
}

void Scan_Init(const ScanInterfaces *interfaces)
{
    io = *interfaces;
    state = SCAN_IDLE;
    continuous = stop_requested = writing = false;
    sequence = 0;
    channel = 0;
    Scan_MakeSafe();
}

static FwStatus Scan_Start(bool repeat)
{
    if (state != SCAN_IDLE) {
        return FW_INVALID_STATE;
    }
    if (FrameBuffer_IsBusy()) {
        return FW_BUSY;
    }
    continuous = repeat;
    stop_requested = false;
    return Scan_PrepareFrame();
}

FwStatus Scan_StartSingle(void)
{
    return Scan_Start(false);
}

FwStatus Scan_StartContinuous(void)
{
    return Scan_Start(true);
}

void Scan_RequestStop(void)
{
    continuous = false;
    stop_requested = true;
    if (state == SCAN_IDLE) {
        Scan_MakeSafe();
    }
}

void Scan_EmergencyStop(void)
{
    continuous = false;
    stop_requested = true;
    Scan_AbortFrame();
    if (state != SCAN_FAULT) {
        state = SCAN_IDLE;
    }
}

FwStatus Scan_ClearFault(void)
{
    if (state != SCAN_FAULT) {
        return FW_INVALID_STATE;
    }
    Scan_AbortFrame();
    Diagnostics_ClearFault();
    Detection_Reset();
    state = SCAN_IDLE;
    return FW_OK;
}

void Scan_OnWaitComplete(void)
{
    if (state == SCAN_SELECT_WAIT) {
        state = SCAN_OFF_ACQUIRE;
    } else if (state == SCAN_LIGHT_WAIT) {
        state = SCAN_ON_ACQUIRE;
    } else {
        Scan_EnterFault(2);
        return;
    }
    if (io.sample(frame_config.sample_us) != FW_OK) {
        Scan_EnterFault(3);
    }
}

static void Scan_BeginLightSettling(void)
{
    io.tx_on();
    state = SCAN_LIGHT_WAIT;
    if (io.wait(frame_config.light_us) != FW_OK) {
        Scan_EnterFault(2);
    }
}

static void Scan_CompleteFrame(void)
{
    Scan_MakeSafe();
    if (FrameBuffer_Publish(write_handle, sequence + 1u) != FW_OK) {
        Scan_EnterFault(3);
        return;
    }
    ++sequence;
    writing = false;
    state = SCAN_IDLE;
}

static void Scan_AdvanceChannel(void)
{
    if (stop_requested) {
        Scan_AbortFrame();
        state = SCAN_IDLE;
    } else if (++channel < SENSOR_CHANNELS) {
        Scan_PrepareChannel();
    } else {
        Scan_CompleteFrame();
    }
}

void Scan_OnSamples(const uint16_t *samples, size_t count)
{
    if (state != SCAN_OFF_ACQUIRE && state != SCAN_ON_ACQUIRE) {
        Scan_EnterFault(3);
        return;
    }
    uint16_t mean;
    if (count != SENSOR_SAMPLES || Signal_CalculateMean(samples, count, &mean) != FW_OK) {
        Scan_EnterFault(4);
        return;
    }
    bool light = state == SCAN_ON_ACQUIRE;
    bool saturated = Signal_CheckSaturation(samples, count);
    if (FrameBuffer_CommitChannel(write_handle, channel, light, mean, saturated) != FW_OK) {
        Scan_EnterFault(3);
        return;
    }
    if (!light) {
        Scan_BeginLightSettling();
    } else {
        io.tx_off();
        Scan_AdvanceChannel();
    }
}

void Scan_Process(void)
{
    if (state != SCAN_IDLE || FrameBuffer_IsBusy()) {
        return;
    }
    if (continuous && !stop_requested) {
        (void)Scan_PrepareFrame();
    } else {
        (void)Config_ApplyAtFrameBoundary();
    }
}

void Scan_GetStatus(AppStatus *status)
{
    DiagnosticStatus diagnostic;
    Diagnostics_GetSnapshot(&diagnostic);
    status->state = state;
    status->channel = channel;
    status->continuous = continuous;
    status->fault = diagnostic.fault;
    status->frames = sequence;
    status->config_version = Config_GetVersion();
    status->rx_dropped = status->tx_dropped = 0;
}
