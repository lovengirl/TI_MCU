#include "frame_buffer.h"
#include <string.h>

static RawFrame frames[2];
static FrameState states[2];
static uint32_t dark_channels[2], light_channels[2];
static unsigned next_write;

void FrameBuffer_Init(void)
{
    memset(frames, 0, sizeof(frames));
    memset(states, 0, sizeof(states));
    memset(dark_channels, 0, sizeof(dark_channels));
    memset(light_channels, 0, sizeof(light_channels));
    next_write = 0;
}

FwStatus FrameBuffer_AcquireWrite(unsigned *handle)
{
    if (handle == NULL) {
        return FW_INVALID_ARGUMENT;
    }
    for (unsigned i = 0; i < 2; ++i) {
        unsigned index = (next_write + i) % 2;
        if (states[index] == FRAME_FREE) {
            states[index] = FRAME_WRITING;
            dark_channels[index] = light_channels[index] = 0;
            *handle = index;
            next_write = index ^ 1u;
            return FW_OK;
        }
    }
    return FW_BUSY;
}

FwStatus FrameBuffer_Begin(unsigned handle, const SensorConfig *config, uint32_t version)
{
    if (handle >= 2 || states[handle] != FRAME_WRITING || config == NULL) {
        return FW_INVALID_STATE;
    }
    memset(&frames[handle], 0, sizeof(frames[handle]));
    frames[handle].config = *config;
    frames[handle].values.config_version = version;
    return FW_OK;
}

FwStatus FrameBuffer_CommitChannel(unsigned handle, unsigned channel, bool light, uint16_t mean, bool saturated)
{
    if (handle >= 2 || channel >= SENSOR_CHANNELS || mean > 4095) {
        return FW_INVALID_ARGUMENT;
    }
    if (states[handle] != FRAME_WRITING) {
        return FW_INVALID_STATE;
    }
    uint32_t bit = 1UL << channel;
    uint32_t *written = light ? &light_channels[handle] : &dark_channels[handle];
    if ((*written & bit) != 0) {
        return FW_INVALID_STATE;
    }
    *written |= bit;
    SensorFrame *frame = &frames[handle].values;
    if (light) {
        frame->on[channel] = mean;
    } else {
        frame->off[channel] = mean;
    }
    if (saturated) {
        frame->saturated |= bit;
    }
    return FW_OK;
}

FwStatus FrameBuffer_Publish(unsigned handle, uint32_t sequence)
{
    if (handle >= 2 || states[handle] != FRAME_WRITING ||
        dark_channels[handle] != SENSOR_ALL || light_channels[handle] != SENSOR_ALL) {
        return FW_INVALID_STATE;
    }
    frames[handle].values.sequence = sequence;
    frames[handle].values.valid = SENSOR_ALL & ~frames[handle].values.saturated;
    states[handle] = FRAME_READY;
    return FW_OK;
}

const RawFrame *FrameBuffer_GetReady(unsigned *handle)
{
    if (handle == NULL) {
        return NULL;
    }
    for (unsigned i = 0; i < 2; ++i) {
        if (states[i] == FRAME_READY) {
            states[i] = FRAME_READING;
            *handle = i;
            return &frames[i];
        }
    }
    return NULL;
}

FwStatus FrameBuffer_Release(unsigned handle)
{
    if (handle >= 2 || states[handle] != FRAME_READING) {
        return FW_INVALID_STATE;
    }
    states[handle] = FRAME_FREE;
    return FW_OK;
}

FwStatus FrameBuffer_Discard(unsigned handle)
{
    if (handle >= 2 || states[handle] != FRAME_WRITING) {
        return FW_INVALID_STATE;
    }
    states[handle] = FRAME_FREE;
    return FW_OK;
}

bool FrameBuffer_IsBusy(void)
{
    return states[0] != FRAME_FREE || states[1] != FRAME_FREE;
}
