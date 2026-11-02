#include "channel_selector.h"
#include "light_control.h"
#include "board_pins.h"
static bool rx_enabled;

void ChannelSelector_Init(void)
{
    ChannelSelector_DisableRx();
}

FwStatus ChannelSelector_Select(uint8_t tx, uint8_t rx)
{
    if (tx >= SENSOR_CHANNELS || rx >= SENSOR_CHANNELS) {
        return FW_INVALID_ARGUMENT;
    }
    if (LightControl_IsEnabled() || rx_enabled) {
        return FW_INVALID_STATE;
    }
    DL_GPIO_writePinsVal(GPIOB, BOARD_TX_ADDRESS_MASK | BOARD_RX_ADDRESS_MASK,
                        tx | ((uint32_t)rx << 6));
    return FW_OK;
}

void ChannelSelector_EnableRx(void)
{
    DL_GPIO_setPins(GPIOB, BOARD_RX_ENABLE);
    rx_enabled = true;
}

void ChannelSelector_DisableRx(void)
{
    DL_GPIO_clearPins(GPIOB, BOARD_RX_ENABLE);
    rx_enabled = false;
}
