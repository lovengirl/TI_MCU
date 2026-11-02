#ifndef LIGHT_CONTROL_H
#define LIGHT_CONTROL_H
#include "sensor_types.h"
void LightControl_Init(void);
FwStatus LightControl_SetLevel(uint16_t code);
void LightControl_EnableTx(void);
void LightControl_DisableTx(void);
void LightControl_ForceOffFromISR(void);
bool LightControl_IsEnabled(void);
#endif
