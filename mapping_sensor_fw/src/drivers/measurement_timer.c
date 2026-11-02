#include "measurement_timer.h"
#include "light_control.h"
#include "ti_msp_dl_config.h"

static volatile uint32_t ticks, deadline;
static volatile bool waiting, wait_complete, armed, timeout_pending;

void MeasurementTimer_Init(void)
{
    ticks = 0;
    waiting = wait_complete = armed = timeout_pending = false;
    NVIC_SetPriority(TIMG0_INT_IRQn, 1);
    NVIC_SetPriority(SysTick_IRQn, 2);
    NVIC_EnableIRQ(TIMG0_INT_IRQn);
    SysTick_Config(CPUCLK_FREQ / 1000u);
}

void MeasurementTimer_ArmDeadline(void)
{
    deadline = ticks + 25u;
    armed = true;
}

void MeasurementTimer_DisarmDeadline(void)
{
    armed = false;
}

FwStatus MeasurementTimer_StartWait(uint16_t us)
{
    if (us < 5 || us > 10000) {
        return FW_INVALID_ARGUMENT;
    }
    if (waiting || wait_complete) {
        return FW_BUSY;
    }
    DL_TimerG_stopCounter(TIMG0);
    DL_TimerG_setLoadValue(TIMG0, us - 1u);
    DL_TimerG_setTimerCount(TIMG0, us - 1u);
    DL_TimerG_clearInterruptStatus(TIMG0, DL_TIMERG_INTERRUPT_ZERO_EVENT);
    NVIC_ClearPendingIRQ(TIMG0_INT_IRQn);
    waiting = true;
    MeasurementTimer_ArmDeadline();
    DL_TimerG_startCounter(TIMG0);
    return FW_OK;
}

void MeasurementTimer_CancelWait(void)
{
    DL_TimerG_stopCounter(TIMG0);
    DL_TimerG_clearInterruptStatus(TIMG0, DL_TIMERG_INTERRUPT_ZERO_EVENT);
    NVIC_ClearPendingIRQ(TIMG0_INT_IRQn);
    waiting = wait_complete = false;
    MeasurementTimer_DisarmDeadline();
}

bool MeasurementTimer_TakeWaitComplete(void)
{
    bool result = wait_complete;
    wait_complete = false;
    return result;
}

void MeasurementTimer_StartSampleClock(uint16_t us)
{
    DL_TimerG_setLoadValue(TIMG6, us - 1u);
    DL_TimerG_setTimerCount(TIMG6, us - 1u);
    MeasurementTimer_ArmDeadline();
    DL_TimerG_startCounter(TIMG6);
}

void MeasurementTimer_StopSampleClock(void)
{
    DL_TimerG_stopCounter(TIMG6);
    MeasurementTimer_DisarmDeadline();
}

void MeasurementTimer_OnWaitIRQ(void)
{
    if (DL_TimerG_getPendingInterrupt(TIMG0) == DL_TIMERG_IIDX_ZERO && waiting) {
        waiting = false;
        wait_complete = true;
        /* Deadline remains armed until main consumes the event. */
    }
}

void MeasurementTimer_OnTickIRQ(void)
{
    ++ticks;
    if (armed && (int32_t)(ticks - deadline) >= 0) {
        LightControl_ForceOffFromISR();
        DL_TimerG_stopCounter(TIMG0);
        DL_TimerG_stopCounter(TIMG6);
        armed = false;
        timeout_pending = true;
    }
}

bool MeasurementTimer_TakeTimeout(void)
{
    bool result = timeout_pending;
    timeout_pending = false;
    return result;
}

uint32_t MeasurementTimer_Millis(void)
{
    return ticks;
}
