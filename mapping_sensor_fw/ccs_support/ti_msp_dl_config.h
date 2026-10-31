/*
 * Copyright (c) 2023, Texas Instruments Incorporated - http://www.ti.com
 * All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 *
 * *  Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 *
 * *  Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in the
 *    documentation and/or other materials provided with the distribution.
 *
 * *  Neither the name of Texas Instruments Incorporated nor the names of
 *    its contributors may be used to endorse or promote products derived
 *    from this software without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS"
 * AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO,
 * THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR
 * PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT OWNER OR
 * CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL,
 * EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO,
 * PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS;
 * OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY,
 * WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR
 * OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE,
 * EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 */

/*
 *  ============ ti_msp_dl_config.h =============
 *  Configured MSPM0 DriverLib module declarations
 *
 *  DO NOT EDIT - This file is generated for the MSPM0G350X
 *  by the SysConfig tool.
 */
#ifndef ti_msp_dl_config_h
#define ti_msp_dl_config_h

#define CONFIG_MSPM0G350X
#define CONFIG_MSPM0G3507

#if defined(__ti_version__) || defined(__TI_COMPILER_VERSION__)
#define SYSCONFIG_WEAK __attribute__((weak))
#elif defined(__IAR_SYSTEMS_ICC__)
#define SYSCONFIG_WEAK __weak
#elif defined(__GNUC__)
#define SYSCONFIG_WEAK __attribute__((weak))
#endif

#include <ti/devices/msp/msp.h>
#include <ti/driverlib/driverlib.h>
#include <ti/driverlib/m0p/dl_core.h>

#ifdef __cplusplus
extern "C" {
#endif

/*
 *  ======== SYSCFG_DL_init ========
 *  Perform all required MSP DL initialization
 *
 *  This function should be called once at a point before any use of
 *  MSP DL.
 */


/* clang-format off */

#define POWER_STARTUP_DELAY                                                (16)



#define CPUCLK_FREQ                                                     80000000
/* Defines for SYSPLL_ERR_01 Workaround */
/* Represent 1.000 as 1000 */
#define FLOAT_TO_INT_SCALE                                               (1000U)
#define FCC_EXPECTED_RATIO                                                  2500
#define FCC_UPPER_BOUND                       (FCC_EXPECTED_RATIO * (1 + 0.003))
#define FCC_LOWER_BOUND                       (FCC_EXPECTED_RATIO * (1 - 0.003))

bool SYSCFG_DL_SYSCTL_SYSPLL_init(void);


/* Defines for WAIT_TIMER */
#define WAIT_TIMER_INST                                                  (TIMG0)
#define WAIT_TIMER_INST_IRQHandler                              TIMG0_IRQHandler
#define WAIT_TIMER_INST_INT_IRQN                                (TIMG0_INT_IRQn)
#define WAIT_TIMER_INST_LOAD_VALUE                                         (49U)
/* Defines for SAMPLE_TIMER */
#define SAMPLE_TIMER_INST                                                (TIMG6)
#define SAMPLE_TIMER_INST_IRQHandler                            TIMG6_IRQHandler
#define SAMPLE_TIMER_INST_INT_IRQN                              (TIMG6_INT_IRQn)
#define SAMPLE_TIMER_INST_LOAD_VALUE                                        (9U)
#define SAMPLE_TIMER_INST_PUB_0_CH                                           (1)



/* Defines for DEBUG_UART */
#define DEBUG_UART_INST                                                    UART0
#define DEBUG_UART_INST_FREQUENCY                                       40000000
#define DEBUG_UART_INST_IRQHandler                              UART0_IRQHandler
#define DEBUG_UART_INST_INT_IRQN                                  UART0_INT_IRQn
#define GPIO_DEBUG_UART_RX_PORT                                            GPIOA
#define GPIO_DEBUG_UART_TX_PORT                                            GPIOA
#define GPIO_DEBUG_UART_RX_PIN                                    DL_GPIO_PIN_11
#define GPIO_DEBUG_UART_TX_PIN                                    DL_GPIO_PIN_10
#define GPIO_DEBUG_UART_IOMUX_RX                                 (IOMUX_PINCM22)
#define GPIO_DEBUG_UART_IOMUX_TX                                 (IOMUX_PINCM21)
#define GPIO_DEBUG_UART_IOMUX_RX_FUNC                  IOMUX_PINCM22_PF_UART0_RX
#define GPIO_DEBUG_UART_IOMUX_TX_FUNC                  IOMUX_PINCM21_PF_UART0_TX
#define DEBUG_UART_BAUD_RATE                                            (115200)
#define DEBUG_UART_IBRD_40_MHZ_115200_BAUD                                  (21)
#define DEBUG_UART_FBRD_40_MHZ_115200_BAUD                                  (45)




/* Defines for HOST_SPI */
#define HOST_SPI_INST                                                      SPI0
#define HOST_SPI_INST_IRQHandler                                SPI0_IRQHandler
#define HOST_SPI_INST_INT_IRQN                                    SPI0_INT_IRQn
#define GPIO_HOST_SPI_PICO_PORT                                           GPIOA
#define GPIO_HOST_SPI_PICO_PIN                                   DL_GPIO_PIN_14
#define GPIO_HOST_SPI_IOMUX_PICO                                (IOMUX_PINCM36)
#define GPIO_HOST_SPI_IOMUX_PICO_FUNC                IOMUX_PINCM36_PF_SPI0_PICO
#define GPIO_HOST_SPI_POCI_PORT                                           GPIOA
#define GPIO_HOST_SPI_POCI_PIN                                   DL_GPIO_PIN_13
#define GPIO_HOST_SPI_IOMUX_POCI                                (IOMUX_PINCM35)
#define GPIO_HOST_SPI_IOMUX_POCI_FUNC                IOMUX_PINCM35_PF_SPI0_POCI
/* GPIO configuration for HOST_SPI */
#define GPIO_HOST_SPI_SCLK_PORT                                           GPIOA
#define GPIO_HOST_SPI_SCLK_PIN                                   DL_GPIO_PIN_12
#define GPIO_HOST_SPI_IOMUX_SCLK                                (IOMUX_PINCM34)
#define GPIO_HOST_SPI_IOMUX_SCLK_FUNC                IOMUX_PINCM34_PF_SPI0_SCLK



/* Defines for RX_ADC */
#define RX_ADC_INST                                                         ADC0
#define RX_ADC_INST_IRQHandler                                   ADC0_IRQHandler
#define RX_ADC_INST_INT_IRQN                                     (ADC0_INT_IRQn)
#define RX_ADC_ADCMEM_0                                       DL_ADC12_MEM_IDX_0
#define RX_ADC_ADCMEM_0_REF                      DL_ADC12_REFERENCE_VOLTAGE_VDDA
#define RX_ADC_ADCMEM_0_REF_VOLTAGE_V                                        3.3
#define RX_ADC_INST_SUB_CH                                                   (1)
#define GPIO_RX_ADC_C0_PORT                                                GPIOA
#define GPIO_RX_ADC_C0_PIN                                        DL_GPIO_PIN_27
#define GPIO_RX_ADC_IOMUX_C0                                     (IOMUX_PINCM60)
#define GPIO_RX_ADC_IOMUX_C0_FUNC                 (IOMUX_PINCM60_PF_UNCONNECTED)



/* Defines for ADC_DMA */
#define ADC_DMA_CHAN_ID                                                      (0)
#define RX_ADC_INST_DMA_TRIGGER                       (DMA_ADC0_EVT_GEN_BD_TRIG)


/* Port definition for Pin Group BOARD */
#define BOARD_PORT                                                       (GPIOB)

/* Defines for TX_A0: GPIOB.0 with pinCMx 12 on package pin 47 */
#define BOARD_TX_A0_PIN                                          (DL_GPIO_PIN_0)
#define BOARD_TX_A0_IOMUX                                        (IOMUX_PINCM12)
/* Defines for TX_A1: GPIOB.1 with pinCMx 13 on package pin 48 */
#define BOARD_TX_A1_PIN                                          (DL_GPIO_PIN_1)
#define BOARD_TX_A1_IOMUX                                        (IOMUX_PINCM13)
/* Defines for TX_A2: GPIOB.2 with pinCMx 15 on package pin 50 */
#define BOARD_TX_A2_PIN                                          (DL_GPIO_PIN_2)
#define BOARD_TX_A2_IOMUX                                        (IOMUX_PINCM15)
/* Defines for TX_A3: GPIOB.3 with pinCMx 16 on package pin 51 */
#define BOARD_TX_A3_PIN                                          (DL_GPIO_PIN_3)
#define BOARD_TX_A3_IOMUX                                        (IOMUX_PINCM16)
/* Defines for TX_A4: GPIOB.4 with pinCMx 17 on package pin 52 */
#define BOARD_TX_A4_PIN                                          (DL_GPIO_PIN_4)
#define BOARD_TX_A4_IOMUX                                        (IOMUX_PINCM17)
/* Defines for TX_EN: GPIOB.5 with pinCMx 18 on package pin 53 */
#define BOARD_TX_EN_PIN                                          (DL_GPIO_PIN_5)
#define BOARD_TX_EN_IOMUX                                        (IOMUX_PINCM18)
/* Defines for RX_A0: GPIOB.6 with pinCMx 23 on package pin 58 */
#define BOARD_RX_A0_PIN                                          (DL_GPIO_PIN_6)
#define BOARD_RX_A0_IOMUX                                        (IOMUX_PINCM23)
/* Defines for RX_A1: GPIOB.7 with pinCMx 24 on package pin 59 */
#define BOARD_RX_A1_PIN                                          (DL_GPIO_PIN_7)
#define BOARD_RX_A1_IOMUX                                        (IOMUX_PINCM24)
/* Defines for RX_A2: GPIOB.8 with pinCMx 25 on package pin 60 */
#define BOARD_RX_A2_PIN                                          (DL_GPIO_PIN_8)
#define BOARD_RX_A2_IOMUX                                        (IOMUX_PINCM25)
/* Defines for RX_A3: GPIOB.9 with pinCMx 26 on package pin 61 */
#define BOARD_RX_A3_PIN                                          (DL_GPIO_PIN_9)
#define BOARD_RX_A3_IOMUX                                        (IOMUX_PINCM26)
/* Defines for RX_A4: GPIOB.10 with pinCMx 27 on package pin 62 */
#define BOARD_RX_A4_PIN                                         (DL_GPIO_PIN_10)
#define BOARD_RX_A4_IOMUX                                        (IOMUX_PINCM27)
/* Defines for RX_EN: GPIOB.11 with pinCMx 28 on package pin 63 */
#define BOARD_RX_EN_PIN                                         (DL_GPIO_PIN_11)
#define BOARD_RX_EN_IOMUX                                        (IOMUX_PINCM28)
/* Defines for HOST_IRQ: GPIOB.12 with pinCMx 29 on package pin 64 */
#define BOARD_HOST_IRQ_PIN                                      (DL_GPIO_PIN_12)
#define BOARD_HOST_IRQ_IOMUX                                     (IOMUX_PINCM29)
/* Defines for HOST_RESET: GPIOB.13 with pinCMx 30 on package pin 1 */
#define BOARD_HOST_RESET_PIN                                    (DL_GPIO_PIN_13)
#define BOARD_HOST_RESET_IOMUX                                   (IOMUX_PINCM30)
/* Defines for STATUS: GPIOB.14 with pinCMx 31 on package pin 2 */
#define BOARD_STATUS_PIN                                        (DL_GPIO_PIN_14)
#define BOARD_STATUS_IOMUX                                       (IOMUX_PINCM31)
/* Defines for SCAN_MARK: GPIOB.15 with pinCMx 32 on package pin 3 */
#define BOARD_SCAN_MARK_PIN                                     (DL_GPIO_PIN_15)
#define BOARD_SCAN_MARK_IOMUX                                    (IOMUX_PINCM32)
/* Defines for RESERVED: GPIOB.16 with pinCMx 33 on package pin 4 */
#define BOARD_RESERVED_PIN                                      (DL_GPIO_PIN_16)
#define BOARD_RESERVED_IOMUX                                     (IOMUX_PINCM33)
/* Port definition for Pin Group HOST */
#define HOST_PORT                                                        (GPIOA)

/* Defines for CS_N: GPIOA.8 with pinCMx 19 on package pin 54 */
#define HOST_CS_N_PIN                                            (DL_GPIO_PIN_8)
#define HOST_CS_N_IOMUX                                          (IOMUX_PINCM19)



/* Defines for DAC12 */
#define DAC12_IRQHandler                                         DAC0_IRQHandler
#define DAC12_INT_IRQN                                           (DAC0_INT_IRQn)
#define GPIO_DAC12_OUT_PORT                                                GPIOA
#define GPIO_DAC12_OUT_PIN                                        DL_GPIO_PIN_15
#define GPIO_DAC12_IOMUX_OUT                                     (IOMUX_PINCM37)
#define GPIO_DAC12_IOMUX_OUT_FUNC                   IOMUX_PINCM37_PF_UNCONNECTED


/* Defines for WWDT */
#define APP_WWDT_INST                                                    (WWDT0)
#define APP_WWDT_INT_IRQN                                       (WWDT0_INT_IRQn)



/* clang-format on */

void SYSCFG_DL_init(void);
void SYSCFG_DL_initPower(void);
void SYSCFG_DL_GPIO_init(void);
void SYSCFG_DL_SYSCTL_init(void);
void SYSCFG_DL_SYSCTL_CLK_init(void);

bool SYSCFG_DL_SYSCTL_SYSPLL_init(void);
void SYSCFG_DL_WAIT_TIMER_init(void);
void SYSCFG_DL_SAMPLE_TIMER_init(void);
void SYSCFG_DL_DEBUG_UART_init(void);
void SYSCFG_DL_HOST_SPI_init(void);
void SYSCFG_DL_RX_ADC_init(void);
void SYSCFG_DL_DMA_init(void);

void SYSCFG_DL_DAC12_init(void);
void SYSCFG_DL_APP_WWDT_init(void);

bool SYSCFG_DL_saveConfiguration(void);
bool SYSCFG_DL_restoreConfiguration(void);

#ifdef __cplusplus
}
#endif

#endif /* ti_msp_dl_config_h */
