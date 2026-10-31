#ifndef PLATFORM_H
#define PLATFORM_H
#include "sensor.h"
extern sensor_t g_sensor;
void platform_init(void);
void platform_poll(void);
uint32_t platform_millis(void);
bool console_get(char *);
bool console_puts(const char *);
extern volatile uint32_t g_uart_rx_overruns, g_uart_tx_dropped;
#endif
