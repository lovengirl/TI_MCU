#ifndef COMMAND_PARSER_H
#define COMMAND_PARSER_H
#include "sensor_types.h"
typedef enum {
    CMD_STATUS, CMD_RESULT, CMD_SINGLE, CMD_START, CMD_STOP, CMD_ESTOP,
    CMD_CLEAR, CMD_DAC, CMD_THRESH, CMD_POLARITY, CMD_HELP, CMD_SAVE
} CommandType;
typedef struct { CommandType type; uint16_t args[3]; } Command;
typedef enum { PARSER_NONE, PARSER_READY, PARSER_ERROR } ParserResult;
void CommandParser_Init(void);
void CommandParser_InvalidateLine(void);
FwStatus CommandParser_Parse(const char *line, Command *command);
ParserResult CommandParser_FeedByte(char byte, Command *command);
#endif
