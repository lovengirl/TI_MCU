#ifndef MEASUREMENT_TIMER_H
#define MEASUREMENT_TIMER_H
#include "sensor_types.h"
void MeasurementTimer_Init(void);
FwStatus MeasurementTimer_StartWait(uint16_t us);
bool MeasurementTimer_TakeWaitComplete(void);
void MeasurementTimer_CancelWait(void);
void MeasurementTimer_StartSampleClock(uint16_t us);
void MeasurementTimer_StopSampleClock(void);
void MeasurementTimer_ArmDeadline(void);
void MeasurementTimer_DisarmDeadline(void);
bool MeasurementTimer_TakeTimeout(void);
uint32_t MeasurementTimer_Millis(void);
void MeasurementTimer_OnWaitIRQ(void);
void MeasurementTimer_OnTickIRQ(void);
#endif
