#include "light_control.h"
#include "board_pins.h"
static volatile bool tx_enabled;

void LightControl_Init(void)
{
    LightControl_DisableTx();
    (void)LightControl_SetLevel(0);
}

FwStatus LightControl_SetLevel(uint16_t code)
{
    if (code > 4095) {
        return FW_INVALID_ARGUMENT;
    }
    if (tx_enabled) {
        return FW_INVALID_STATE;
    }
    DL_DAC12_output12(DAC0, code);
    return FW_OK;
}

void LightControl_EnableTx(void)
{
    tx_enabled = true;
    DL_GPIO_setPins(GPIOB, BOARD_TX_ENABLE);
}

void LightControl_DisableTx(void)
{
    DL_GPIO_clearPins(GPIOB, BOARD_TX_ENABLE);
    tx_enabled = false;
}

void LightControl_ForceOffFromISR(void)
{
    LightControl_DisableTx();
}

bool LightControl_IsEnabled(void)
{
    return tx_enabled;
}
