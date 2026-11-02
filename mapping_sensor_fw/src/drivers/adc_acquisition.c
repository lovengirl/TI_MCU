#include "adc_acquisition.h"
#include "measurement_timer.h"
#include "light_control.h"
#include "board_pins.h"

typedef enum { ACQ_IDLE, ACQ_ARMED, ACQ_COMPLETE, ACQ_READING } AcquisitionState;
static volatile AcquisitionState state;
static uint16_t samples[SENSOR_SAMPLES];
static volatile uint32_t generation, completed_generation;

static void Acquisition_StopHardware(void)
{
    MeasurementTimer_StopSampleClock();
    DL_ADC12_disableConversions(ADC0);
    DL_ADC12_stopConversion(ADC0);
    DL_ADC12_disableDMA(ADC0);
    DL_DMA_disableChannel(DMA, 0);
    DL_DMA_clearInterruptStatus(DMA, DL_DMA_INTERRUPT_CHANNEL0);
    NVIC_ClearPendingIRQ(DMA_INT_IRQn);
    DL_GPIO_clearPins(GPIOB, BOARD_SCAN_MARK_PIN);
}

void Acquisition_Init(void)
{
    generation = completed_generation = 0;
    Acquisition_Cancel();
    DL_DMA_enableInterrupt(DMA, DL_DMA_INTERRUPT_CHANNEL0);
    NVIC_SetPriority(DMA_INT_IRQn, 0);
    NVIC_EnableIRQ(DMA_INT_IRQn);
}

FwStatus Acquisition_Start(uint16_t period_us)
{
    if (period_us < 10 || period_us > 1000) {
        return FW_INVALID_ARGUMENT;
    }
    if (state != ACQ_IDLE) {
        return FW_BUSY;
    }
    Acquisition_StopHardware();
    /* Never rearm over a conversion still finishing after cancellation. */
    if ((DL_ADC12_getStatus(ADC0) & DL_ADC12_STATUS_CONVERSION_ACTIVE) != 0) {
        return FW_BUSY;
    }
    DL_ADC12_clearDMATriggerStatus(ADC0, DL_ADC12_DMA_MEM0_RESULT_LOADED);
    DL_ADC12_clearInterruptStatus(ADC0, DL_ADC12_INTERRUPT_DMA_DONE);
    ++generation;
    DL_ADC12_setDMASamplesCnt(ADC0, 1);
    DL_DMA_setSrcAddr(DMA, 0, DL_ADC12_getMemResultAddress(ADC0, DL_ADC12_MEM_IDX_0));
    DL_DMA_setDestAddr(DMA, 0, (uint32_t)samples);
    DL_DMA_setTransferSize(DMA, 0, SENSOR_SAMPLES);
    DL_ADC12_enableDMA(ADC0);
    DL_DMA_enableChannel(DMA, 0);
    DL_ADC12_enableConversions(ADC0);
    state = ACQ_ARMED;
    DL_GPIO_setPins(GPIOB, BOARD_SCAN_MARK_PIN);
    MeasurementTimer_StartSampleClock(period_us);
    return FW_OK;
}

void Acquisition_OnDMAIRQ(void)
{
    if (!DL_DMA_getRawInterruptStatus(DMA, DL_DMA_INTERRUPT_CHANNEL0)) {
        return;
    }
    bool expected = state == ACQ_ARMED;
    LightControl_ForceOffFromISR();
    Acquisition_StopHardware();
    if (expected) {
        completed_generation = generation;
        state = ACQ_COMPLETE;
    }
}

const uint16_t *Acquisition_GetSamples(uint32_t *id)
{
    if (state != ACQ_COMPLETE || completed_generation != generation) {
        return NULL;
    }
    state = ACQ_READING;
    *id = completed_generation;
    return samples;
}

void Acquisition_Release(void)
{
    if (state == ACQ_READING) {
        state = ACQ_IDLE;
    }
}

void Acquisition_Cancel(void)
{
    Acquisition_StopHardware();
    ++generation;
    state = ACQ_IDLE;
}
