#include "board.h"
#include "board_pins.h"

void Board_Init(void)
{
    /* Generated GPIO setup establishes inactive outputs before clocks/analog init. */
    SYSCFG_DL_init();
    DL_GPIO_clearPins(GPIOB, BOARD_TX_ENABLE | BOARD_RX_ENABLE | BOARD_SCAN_MARK_PIN);
    DL_DAC12_output12(DAC0, 0);
}

uint32_t Board_EnterCritical(void)
{
    uint32_t key = __get_PRIMASK();
    __disable_irq();
    return key;
}

void Board_ExitCritical(uint32_t key)
{
    __set_PRIMASK(key);
}

void Board_SetFaultIndicator(bool active)
{
    if (active) {
        DL_GPIO_setPins(GPIOB, BOARD_STATUS_PIN);
    } else {
        DL_GPIO_clearPins(GPIOB, BOARD_STATUS_PIN);
    }
}
