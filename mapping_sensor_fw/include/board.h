#ifndef BOARD_H
#define BOARD_H
#include "sensor_types.h"
void Board_Init(void);
uint32_t Board_EnterCritical(void);
void Board_ExitCritical(uint32_t key);
void Board_SetFaultIndicator(bool active);
#endif
