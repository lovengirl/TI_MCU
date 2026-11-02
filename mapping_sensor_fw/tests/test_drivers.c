/* Register-contract tests execute production driver C on a memory-backed MMIO map.
 * Registers are injected explicitly; this does not emulate the MSPM0 peripherals. */
#include "board_pins.h"
#include "light_control.h"
#include "channel_selector.h"
#include "adc_acquisition.h"
#include "measurement_timer.h"
#include "uart_transport.h"
#include "watchdog_service.h"
#include <string.h>

volatile uint32_t test_checks, test_fail_line, test_cases;
#define CHECK(condition) do { ++test_checks; if (!(condition)) { test_fail_line = __LINE__; return false; } } while (0)

static void InjectRegister(const volatile void *address, uint32_t value)
{
    *(volatile uint32_t *)(uintptr_t)address = value;
}

static bool TestLightAndSelector(void)
{
    LightControl_Init(); ChannelSelector_Init();
    CHECK(!LightControl_IsEnabled());
    CHECK(LightControl_SetLevel(4096) == FW_INVALID_ARGUMENT);
    CHECK(LightControl_SetLevel(512) == FW_OK);
    CHECK(ChannelSelector_Select(19, 19) == FW_OK);
    CHECK(ChannelSelector_Select(20, 0) == FW_INVALID_ARGUMENT);
    ChannelSelector_EnableRx();
    CHECK(ChannelSelector_Select(0, 0) == FW_INVALID_STATE);
    ChannelSelector_DisableRx(); LightControl_EnableTx();
    CHECK(LightControl_IsEnabled());
    CHECK(ChannelSelector_Select(0, 0) == FW_INVALID_STATE);
    CHECK(LightControl_SetLevel(0) == FW_INVALID_STATE);
    LightControl_ForceOffFromISR();
    CHECK(!LightControl_IsEnabled());
    CHECK(ChannelSelector_Select(0, 0) == FW_OK);
    return true;
}

static bool TestTimerCompletion(void)
{
    MeasurementTimer_Init();
    CHECK(MeasurementTimer_StartWait(0) == FW_INVALID_ARGUMENT);
    CHECK(MeasurementTimer_StartWait(50) == FW_OK);
    CHECK(MeasurementTimer_StartWait(50) == FW_BUSY);
    InjectRegister(&TIMG0->CPU_INT.IIDX, DL_TIMERG_IIDX_ZERO);
    MeasurementTimer_OnWaitIRQ();
    InjectRegister(&TIMG0->CPU_INT.IIDX, 0);
    CHECK(MeasurementTimer_TakeWaitComplete());
    CHECK(!MeasurementTimer_TakeWaitComplete());
    MeasurementTimer_CancelWait();
    InjectRegister(&TIMG0->CPU_INT.IIDX, DL_TIMERG_IIDX_ZERO);
    MeasurementTimer_OnWaitIRQ();
    InjectRegister(&TIMG0->CPU_INT.IIDX, 0);
    CHECK(!MeasurementTimer_TakeWaitComplete());
    return true;
}

static bool TestTimeoutCutoff(void)
{
    MeasurementTimer_Init(); LightControl_EnableTx();
    CHECK(MeasurementTimer_StartWait(50) == FW_OK);
    for (unsigned i = 0; i < 24; ++i) { MeasurementTimer_OnTickIRQ(); }
    CHECK(!MeasurementTimer_TakeTimeout() && LightControl_IsEnabled());
    MeasurementTimer_OnTickIRQ();
    CHECK(MeasurementTimer_TakeTimeout() && !LightControl_IsEnabled());
    CHECK(!MeasurementTimer_TakeTimeout());
    CHECK(MeasurementTimer_Millis() == 25);
    MeasurementTimer_CancelWait();
    CHECK(MeasurementTimer_StartWait(50) == FW_OK);
    MeasurementTimer_CancelWait();
    return true;
}

static bool TestAcquisitionLifecycle(void)
{
    InjectRegister(&ADC0->ULLMEM.STATUS, 0);
    InjectRegister(&DMA->CPU_INT.RIS, 0);
    Acquisition_Init();
    CHECK(Acquisition_Start(9) == FW_INVALID_ARGUMENT);
    CHECK(Acquisition_Start(10) == FW_OK);
    CHECK(Acquisition_Start(10) == FW_BUSY);
    uint32_t id;
    CHECK(Acquisition_GetSamples(&id) == NULL);
    InjectRegister(&DMA->CPU_INT.RIS, DL_DMA_INTERRUPT_CHANNEL0);
    Acquisition_OnDMAIRQ();
    InjectRegister(&DMA->CPU_INT.RIS, 0);
    CHECK(Acquisition_GetSamples(&id) != NULL);
    uint32_t first_id = id;
    CHECK(Acquisition_GetSamples(&id) == NULL);
    CHECK(Acquisition_Start(10) == FW_BUSY);
    Acquisition_Release();
    CHECK(Acquisition_Start(10) == FW_OK);
    InjectRegister(&DMA->CPU_INT.RIS, DL_DMA_INTERRUPT_CHANNEL0);
    Acquisition_OnDMAIRQ();
    InjectRegister(&DMA->CPU_INT.RIS, 0);
    CHECK(Acquisition_GetSamples(&id) != NULL && id > first_id);
    Acquisition_Cancel();
    CHECK(Acquisition_GetSamples(&id) == NULL);
    return true;
}

static bool TestCancelledDMA(void)
{
    Acquisition_Cancel();
    InjectRegister(&DMA->CPU_INT.RIS, DL_DMA_INTERRUPT_CHANNEL0);
    Acquisition_OnDMAIRQ();
    InjectRegister(&DMA->CPU_INT.RIS, 0);
    uint32_t id;
    CHECK(Acquisition_GetSamples(&id) == NULL);
    InjectRegister(&ADC0->ULLMEM.STATUS, DL_ADC12_STATUS_CONVERSION_ACTIVE);
    CHECK(Acquisition_Start(10) == FW_BUSY);
    InjectRegister(&ADC0->ULLMEM.STATUS, 0);
    CHECK(Acquisition_Start(10) == FW_OK);
    Acquisition_Cancel();
    return true;
}

static bool TestUartBackpressure(void)
{
    UartTransport_Init();
    char byte;
    CHECK(!UartTransport_ReadByte(&byte));
    static char large[2050];
    memset(large, 'x', sizeof(large) - 1u); large[2049] = '\0';
    CHECK(UartTransport_TryWrite(large) == FW_INVALID_ARGUMENT);
    large[2040] = '\0';
    CHECK(UartTransport_TryWrite(large) == FW_OK);
    CHECK(UartTransport_TryWrite("0123456789") == FW_BUSY);
    for (unsigned i = 0; i < 20; ++i) { UartTransport_ProcessTx(); }
    CHECK(UartTransport_TryWrite("0123456789") == FW_OK);
    uint32_t rx, tx;
    UartTransport_GetCounters(&rx, &tx);
    CHECK(rx == 0 && tx == 1);
    return true;
}

uint32_t tests_run(void)
{
    test_checks = test_fail_line = test_cases = 0;
    bool (*cases[])(void) = {
        TestLightAndSelector, TestTimerCompletion, TestTimeoutCutoff,
        TestAcquisitionLifecycle, TestCancelledDMA, TestUartBackpressure
    };
    for (unsigned i = 0; i < sizeof(cases) / sizeof(cases[0]); ++i) {
        if (!cases[i]()) { return i + 1u; }
        ++test_cases;
    }
    return 0;
}
