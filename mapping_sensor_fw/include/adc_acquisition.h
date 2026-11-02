#ifndef ADC_ACQUISITION_H
#define ADC_ACQUISITION_H
#include "sensor_types.h"
void Acquisition_Init(void);
FwStatus Acquisition_Start(uint16_t period_us);
const uint16_t *Acquisition_GetSamples(uint32_t *generation);
void Acquisition_Release(void);
void Acquisition_Cancel(void);
void Acquisition_OnDMAIRQ(void);
#endif
