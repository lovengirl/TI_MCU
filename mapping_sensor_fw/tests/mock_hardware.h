#ifndef MOCK_HARDWARE_H
#define MOCK_HARDWARE_H
#include "sensor_types.h"
void Mock_CompleteWait(void);
void Mock_CompleteSamples(uint16_t value);
void Mock_Timeout(void);
void Mock_QueueInput(const char *text);
void Mock_ClearOutput(void);
const char *Mock_Output(void);
void Mock_BlockOutput(bool blocked);
void Mock_SetRxOverrun(void);
unsigned Mock_Violations(void);
uint8_t Mock_Channel(void);
uint16_t Mock_Dac(void);
bool Mock_Tx(void);
bool Mock_Rx(void);
bool Mock_Waiting(void);
bool Mock_Sampling(void);
#endif
