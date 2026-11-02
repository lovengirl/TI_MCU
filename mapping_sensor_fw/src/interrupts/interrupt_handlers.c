#include "measurement_timer.h"
#include "adc_acquisition.h"
#include "uart_transport.h"

void TIMG0_IRQHandler(void)
{
    MeasurementTimer_OnWaitIRQ();
}

void DMA_IRQHandler(void)
{
    Acquisition_OnDMAIRQ();
}

void UART0_IRQHandler(void)
{
    UartTransport_OnIRQ();
}

void SysTick_Handler(void)
{
    MeasurementTimer_OnTickIRQ();
}
