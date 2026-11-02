#include "response_formatter.h"
#include <stdio.h>

static FwStatus (*write_text)(const char *);
static CommandReply snapshot;
static bool active;
static unsigned line_index;
static bool acknowledgements[16];
static unsigned ack_head, ack_tail;
static uint32_t dropped;

void ResponseFormatter_Init(FwStatus (*writer)(const char *))
{
    write_text = writer;
    active = false;
    line_index = ack_head = ack_tail = 0;
    dropped = 0;
}

bool ResponseFormatter_IsBusy(void)
{
    return active || ack_head != ack_tail;
}

FwStatus ResponseFormatter_Begin(const CommandReply *reply)
{
    if (ResponseFormatter_IsBusy()) {
        return FW_BUSY;
    }
    snapshot = *reply;
    line_index = 0;
    active = true;
    return FW_OK;
}

void ResponseFormatter_QueueAck(bool ok)
{
    unsigned next = (ack_head + 1u) % 16u;
    if (next == ack_tail) {
        ++dropped;
        return;
    }
    acknowledgements[ack_head] = ok;
    ack_head = next;
}

static int FormatFrameLine(char *text, size_t capacity)
{
    const SensorFrame *frame = &snapshot.frame;
    if (line_index == 0) {
        return snprintf(text, capacity, "FRAME=%lu CFG=%lu VALID=%05lX DETECT=%05lX SAT=%05lX\r\n",
            (unsigned long)frame->sequence, (unsigned long)frame->config_version,
            (unsigned long)frame->valid, (unsigned long)frame->detected, (unsigned long)frame->saturated);
    }
    unsigned channel = line_index - 1u;
    return snprintf(text, capacity, "CH=%u OFF=%u ON=%u SIG=%ld\r\n", channel,
        frame->off[channel], frame->on[channel], (long)frame->signal[channel]);
}

static int FormatStatus(char *text, size_t capacity)
{
    const AppStatus *status = &snapshot.status;
    return snprintf(text, capacity,
        "STATE=%u FAULT=%lu FRAME=%lu CFG=%lu RX_DROP=%lu TX_DROP=%lu\r\n",
        (unsigned)status->state, (unsigned long)status->fault, (unsigned long)status->frames,
        (unsigned long)status->config_version, (unsigned long)status->rx_dropped,
        (unsigned long)status->tx_dropped);
}

void ResponseFormatter_Process(void)
{
    if (!active) {
        if (ack_head != ack_tail && write_text(acknowledgements[ack_tail] ? "OK\r\n" : "ERR\r\n") == FW_OK) {
            ack_tail = (ack_tail + 1u) % 16u;
        }
        return;
    }
    char text[192];
    int length = 0;
    switch (snapshot.type) {
    case REPLY_FRAME:
        length = FormatFrameLine(text, sizeof(text));
        break;
    case REPLY_STATUS:
        length = FormatStatus(text, sizeof(text));
        break;
    case REPLY_ACK:
        length = snprintf(text, sizeof(text), "%s\r\n", snapshot.ok ? "OK" : "ERR");
        break;
    case REPLY_NO_DATA:
        length = snprintf(text, sizeof(text), "NO_DATA\r\n");
        break;
    case REPLY_SAVE:
        length = snprintf(text, sizeof(text), "UNSUPPORTED: RAM settings only\r\n");
        break;
    case REPLY_HELP:
        length = snprintf(text, sizeof(text), "STATUS RESULT SINGLE START STOP ESTOP CLEAR DAC ch code THRESH ch enter leave POLARITY 0|1 HELP\r\n");
        break;
    }
    if (length < 0 || (size_t)length >= sizeof(text)) {
        ++dropped;
        active = false;
        return;
    }
    if (write_text(text) != FW_OK) {
        return;
    }
    ++line_index;
    if (snapshot.type != REPLY_FRAME || line_index > SENSOR_CHANNELS) {
        active = false;
    }
}

uint32_t ResponseFormatter_GetDrops(void)
{
    return dropped;
}
