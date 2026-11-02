#ifndef UART_TRANSPORT_H
#define UART_TRANSPORT_H
#include "sensor_types.h"
void UartTransport_Init(void);
bool UartTransport_ReadByte(char *byte);
FwStatus UartTransport_TryWrite(const char *text);
void UartTransport_ProcessTx(void);
void UartTransport_OnIRQ(void);
void UartTransport_GetCounters(uint32_t *rx, uint32_t *tx);
#endif
