#ifndef RESPONSE_FORMATTER_H
#define RESPONSE_FORMATTER_H
#include "sensor_types.h"
typedef enum { REPLY_ACK, REPLY_STATUS, REPLY_FRAME, REPLY_NO_DATA, REPLY_HELP, REPLY_SAVE } ReplyType;
typedef struct {
    ReplyType type;
    bool ok;
    AppStatus status;
    SensorFrame frame;
} CommandReply;
void ResponseFormatter_Init(FwStatus (*writer)(const char *));
bool ResponseFormatter_IsBusy(void);
FwStatus ResponseFormatter_Begin(const CommandReply *reply);
void ResponseFormatter_QueueAck(bool ok);
void ResponseFormatter_Process(void);
uint32_t ResponseFormatter_GetDrops(void);
#endif
