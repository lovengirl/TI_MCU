#ifndef CHANNEL_SELECTOR_H
#define CHANNEL_SELECTOR_H
#include "sensor_types.h"
void ChannelSelector_Init(void);
FwStatus ChannelSelector_Select(uint8_t tx, uint8_t rx);
void ChannelSelector_EnableRx(void);
void ChannelSelector_DisableRx(void);
#endif
