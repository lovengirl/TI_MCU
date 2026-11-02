#include "detection.h"
#include <string.h>

static uint8_t confirmation_count[SENSOR_CHANNELS];
static uint32_t detected;
static uint32_t previous_version;

void Detection_Reset(void)
{
    memset(confirmation_count, 0, sizeof(confirmation_count));
    detected = 0;
    previous_version = 0;
}

void Detection_UpdateChannel(unsigned channel, int32_t signal, bool valid, const SensorConfig *config)
{
    if (channel >= SENSOR_CHANNELS) {
        return;
    }
    uint32_t bit = 1UL << channel;
    if (!valid) {
        confirmation_count[channel] = 0;
        detected &= ~bit;
        return;
    }
    bool active = (detected & bit) != 0;
    bool change = active ? signal <= config->leave[channel] : signal >= config->enter[channel];
    if (!change) {
        confirmation_count[channel] = 0;
    } else if (++confirmation_count[channel] >= config->confirm) {
        detected ^= bit;
        confirmation_count[channel] = 0;
    }
}

void Detection_UpdateFrame(SensorFrame *frame, const SensorConfig *config)
{
    if (previous_version != frame->config_version) {
        Detection_Reset();
        previous_version = frame->config_version;
    }
    for (unsigned i = 0; i < SENSOR_CHANNELS; ++i) {
        Detection_UpdateChannel(i, frame->signal[i], (frame->valid & (1UL << i)) != 0, config);
    }
    frame->detected = detected & frame->valid;
}

uint32_t Detection_GetBitmap(void)
{
    return detected;
}
