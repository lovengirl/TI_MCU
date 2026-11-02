#ifndef FRAME_BUFFER_H
#define FRAME_BUFFER_H
#include "sensor_types.h"
typedef enum { FRAME_FREE, FRAME_WRITING, FRAME_READY, FRAME_READING } FrameState;
void FrameBuffer_Init(void);
FwStatus FrameBuffer_AcquireWrite(unsigned *handle);
FwStatus FrameBuffer_Begin(unsigned handle, const SensorConfig *config, uint32_t version);
FwStatus FrameBuffer_CommitChannel(unsigned handle, unsigned channel, bool light, uint16_t mean, bool saturated);
FwStatus FrameBuffer_Publish(unsigned handle, uint32_t sequence);
const RawFrame *FrameBuffer_GetReady(unsigned *handle);
FwStatus FrameBuffer_Release(unsigned handle);
FwStatus FrameBuffer_Discard(unsigned handle);
bool FrameBuffer_IsBusy(void);
#endif
