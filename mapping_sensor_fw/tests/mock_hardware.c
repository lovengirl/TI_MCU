/* Deterministic hardware contract model. No analog/timing simulation is claimed. */
#include "mock_hardware.h"
#include "board.h"
#include "channel_selector.h"
#include "light_control.h"
#include "measurement_timer.h"
#include "adc_acquisition.h"
#include "uart_transport.h"
#include "watchdog_service.h"
#include <string.h>

static bool tx, rx, waiting, sampling, wait_done, sample_done, timeout, block_output;
static uint8_t selected;
static uint16_t level, samples[SENSOR_SAMPLES];
static unsigned violations;
static uint32_t generation, rx_overrun;
static char input[1024], output[8192];
static size_t input_read, input_write, output_size;

void Board_Init(void)
{
    tx = rx = waiting = sampling = wait_done = sample_done = timeout = block_output = false;
    selected = 0;
    level = 0;
    violations = 0;
    generation = rx_overrun = 0;
    input_read = input_write = output_size = 0;
    output[0] = '\0';
}
uint32_t Board_EnterCritical(void) { return 0; }
void Board_ExitCritical(uint32_t key) { (void)key; }
void Board_SetFaultIndicator(bool active) { (void)active; }
void LightControl_Init(void) { tx = false; level = 0; }
bool LightControl_IsEnabled(void) { return tx; }
void LightControl_EnableTx(void) { tx = true; }
void LightControl_DisableTx(void) { tx = false; }
void LightControl_ForceOffFromISR(void) { tx = false; }
FwStatus LightControl_SetLevel(uint16_t code)
{
    if (tx || code > 4095) { ++violations; return FW_INVALID_ARGUMENT; }
    level = code;
    return FW_OK;
}
void ChannelSelector_Init(void) { rx = false; }
void ChannelSelector_EnableRx(void) { rx = true; }
void ChannelSelector_DisableRx(void) { rx = false; }
FwStatus ChannelSelector_Select(uint8_t tx_channel, uint8_t rx_channel)
{
    if (tx || rx || tx_channel != rx_channel || tx_channel >= SENSOR_CHANNELS) {
        ++violations;
        return FW_INVALID_ARGUMENT;
    }
    selected = tx_channel;
    return FW_OK;
}
void MeasurementTimer_Init(void) { waiting = wait_done = timeout = false; }
FwStatus MeasurementTimer_StartWait(uint16_t us)
{
    if (waiting || us < 5 || us > 10000) { return FW_BUSY; }
    waiting = true;
    return FW_OK;
}
void MeasurementTimer_CancelWait(void) { waiting = wait_done = false; }
bool MeasurementTimer_TakeWaitComplete(void)
{
    bool event = wait_done;
    wait_done = false;
    return event;
}
bool MeasurementTimer_TakeTimeout(void)
{
    bool event = timeout;
    timeout = false;
    return event;
}
void Acquisition_Init(void) { sampling = sample_done = false; }
FwStatus Acquisition_Start(uint16_t period_us)
{
    if (sampling || sample_done || period_us < 10) { return FW_BUSY; }
    sampling = true;
    ++generation;
    return FW_OK;
}
const uint16_t *Acquisition_GetSamples(uint32_t *id)
{
    if (!sample_done) { return NULL; }
    *id = generation;
    return samples;
}
void Acquisition_Release(void) { sample_done = false; }
void Acquisition_Cancel(void) { sampling = sample_done = false; ++generation; }
void UartTransport_Init(void) {}
bool UartTransport_ReadByte(char *byte)
{
    if (input_read == input_write) { return false; }
    *byte = input[input_read++ % sizeof(input)];
    return true;
}
FwStatus UartTransport_TryWrite(const char *text)
{
    size_t size = strlen(text);
    if (block_output || output_size + size >= sizeof(output)) { return FW_BUSY; }
    memcpy(output + output_size, text, size + 1);
    output_size += size;
    return FW_OK;
}
void UartTransport_ProcessTx(void) {}
void UartTransport_GetCounters(uint32_t *rx_count, uint32_t *tx_count)
{
    *rx_count = rx_overrun;
    *tx_count = 0;
}
void WatchdogService_Init(void) {}
void WatchdogService_Process(void) {}
void Mock_CompleteWait(void)
{
    if (!waiting) { ++violations; }
    waiting = false;
    wait_done = true;
}
void Mock_CompleteSamples(uint16_t value)
{
    if (!sampling) { ++violations; }
    for (unsigned i = 0; i < SENSOR_SAMPLES; ++i) { samples[i] = value; }
    sampling = false;
    tx = false;
    sample_done = true;
}
void Mock_Timeout(void) { timeout = true; tx = false; }
void Mock_QueueInput(const char *text)
{
    while (*text != '\0') { input[input_write++ % sizeof(input)] = *text++; }
}
void Mock_ClearOutput(void) { output_size = 0; output[0] = '\0'; }
const char *Mock_Output(void) { return output; }
void Mock_BlockOutput(bool blocked) { block_output = blocked; }
void Mock_SetRxOverrun(void) { ++rx_overrun; }
unsigned Mock_Violations(void) { return violations; }
uint8_t Mock_Channel(void) { return selected; }
uint16_t Mock_Dac(void) { return level; }
bool Mock_Tx(void) { return tx; }
bool Mock_Rx(void) { return rx; }
bool Mock_Waiting(void) { return waiting; }
bool Mock_Sampling(void) { return sampling; }
