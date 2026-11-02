#ifndef DETECTION_H
#define DETECTION_H
#include "sensor_types.h"
void Detection_Reset(void);
void Detection_UpdateChannel(unsigned channel, int32_t signal, bool valid, const SensorConfig *config);
void Detection_UpdateFrame(SensorFrame *frame, const SensorConfig *config);
uint32_t Detection_GetBitmap(void);
#endif
