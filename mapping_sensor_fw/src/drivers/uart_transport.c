#include "uart_transport.h"
#include "ti_msp_dl_config.h"
#include <string.h>

static char rx_buffer[128], tx_buffer[2048];
static volatile uint16_t rx_head, rx_tail;
static uint16_t tx_head, tx_tail;
static volatile uint32_t rx_overruns;
static uint32_t tx_rejected;

void UartTransport_Init(void)
{
    rx_head = rx_tail = tx_head = tx_tail = 0;
    rx_overruns = tx_rejected = 0;
    NVIC_SetPriority(UART0_INT_IRQn, 3);
    NVIC_EnableIRQ(UART0_INT_IRQn);
}

void UartTransport_OnIRQ(void)
{
    (void)DL_UART_Main_getPendingInterrupt(UART0);
    while (!DL_UART_Main_isRXFIFOEmpty(UART0)) {
        char byte = (char)DL_UART_Main_receiveData(UART0);
        uint16_t next = (rx_head + 1u) % sizeof(rx_buffer);
        if (next == rx_tail) {
            ++rx_overruns;
        } else {
            rx_buffer[rx_head] = byte;
            __DMB();
            rx_head = next;
        }
    }
}

bool UartTransport_ReadByte(char *byte)
{
    if (rx_head == rx_tail) {
        return false;
    }
    *byte = rx_buffer[rx_tail];
    rx_tail = (rx_tail + 1u) % sizeof(rx_buffer);
    return true;
}

FwStatus UartTransport_TryWrite(const char *text)
{
    size_t length = strlen(text);
    size_t available = (tx_tail + sizeof(tx_buffer) - tx_head - 1u) % sizeof(tx_buffer);
    if (length > available) {
        /* Backpressure is retried by the formatter; only oversized messages are lost. */
        if (length >= sizeof(tx_buffer)) {
            ++tx_rejected;
            return FW_INVALID_ARGUMENT;
        }
        return FW_BUSY;
    }
    while (*text != '\0') {
        tx_buffer[tx_head] = *text++;
        tx_head = (tx_head + 1u) % sizeof(tx_buffer);
    }
    return FW_OK;
}

void UartTransport_ProcessTx(void)
{
    if (tx_tail != tx_head && !DL_UART_Main_isTXFIFOFull(UART0)) {
        DL_UART_Main_transmitData(UART0, (uint8_t)tx_buffer[tx_tail]);
        tx_tail = (tx_tail + 1u) % sizeof(tx_buffer);
    }
}

void UartTransport_GetCounters(uint32_t *rx, uint32_t *tx)
{
    *rx = rx_overruns;
    *tx = tx_rejected;
}
