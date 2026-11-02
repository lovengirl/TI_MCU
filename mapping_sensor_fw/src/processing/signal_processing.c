#include "signal_processing.h"

FwStatus Signal_CalculateMean(const uint16_t *samples, size_t count, uint16_t *mean)
{
    uint32_t sum = 0;
    if (samples == NULL || mean == NULL || count == 0 || count > SENSOR_SAMPLES) {
        return FW_INVALID_ARGUMENT;
    }
    for (size_t i = 0; i < count; ++i) {
        if (samples[i] > 4095) {
            return FW_INVALID_ARGUMENT;
        }
        sum += samples[i];
    }
    *mean = (uint16_t)(sum / count);
    return FW_OK;
}

bool Signal_CheckSaturation(const uint16_t *samples, size_t count)
{
    for (size_t i = 0; i < count; ++i) {
        if (samples[i] == 4095) {
            return true;
        }
    }
    return false;
}

int32_t Signal_CalculateDifference(uint16_t off, uint16_t on, uint8_t polarity)
{
    return polarity ? (int32_t)on - off : (int32_t)off - on;
}

int32_t Signal_ApplyCalibration(int32_t value, int16_t offset, uint16_t gain_q12)
{
    /* ADC/config bounds ensure the signed 32-bit product cannot overflow. */
    return (value - offset) * (int32_t)gain_q12 / 4096;
}

void Signal_ProcessFrame(const RawFrame *raw, SensorFrame *output)
{
    *output = raw->values;
    for (unsigned i = 0; i < SENSOR_CHANNELS; ++i) {
        int32_t difference = Signal_CalculateDifference(output->off[i], output->on[i], raw->config.polarity);
        output->signal[i] = Signal_ApplyCalibration(difference, raw->config.offset[i], raw->config.gain_q12[i]);
    }
}
