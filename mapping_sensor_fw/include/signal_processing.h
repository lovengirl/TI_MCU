#ifndef SIGNAL_PROCESSING_H
#define SIGNAL_PROCESSING_H
#include "sensor_types.h"
FwStatus Signal_CalculateMean(const uint16_t *samples, size_t count, uint16_t *mean);
bool Signal_CheckSaturation(const uint16_t *samples, size_t count);
int32_t Signal_CalculateDifference(uint16_t off, uint16_t on, uint8_t polarity);
int32_t Signal_ApplyCalibration(int32_t value, int16_t offset, uint16_t gain_q12);
void Signal_ProcessFrame(const RawFrame *raw, SensorFrame *output);
#endif
