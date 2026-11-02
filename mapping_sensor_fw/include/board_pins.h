#ifndef BOARD_PINS_H
#define BOARD_PINS_H
#include "ti_msp_dl_config.h"
/* LQFP64: TX PB0..4=47,48,50,51,52; RX PB6..10=58..62. */
#define BOARD_TX_ADDRESS_MASK 0x1fu
#define BOARD_RX_ADDRESS_MASK (0x1fu << 6)
#define BOARD_TX_ENABLE BOARD_TX_EN_PIN /* PB5, physical 53, high active */
#define BOARD_RX_ENABLE BOARD_RX_EN_PIN /* PB11, physical 63, high active */
#endif
