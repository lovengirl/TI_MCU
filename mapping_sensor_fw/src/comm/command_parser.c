#include "command_parser.h"
#include <string.h>

static char line_buffer[96];
static size_t line_length;
static bool invalid_line;
typedef struct {
    const char *name;
    CommandType type;
    unsigned count;
    uint16_t limits[3];
} CommandDefinition;
static const CommandDefinition definitions[] = {
    {"STATUS", CMD_STATUS, 0, {0}}, {"RESULT", CMD_RESULT, 0, {0}},
    {"SINGLE", CMD_SINGLE, 0, {0}}, {"START", CMD_START, 0, {0}},
    {"STOP", CMD_STOP, 0, {0}}, {"ESTOP", CMD_ESTOP, 0, {0}},
    {"CLEAR", CMD_CLEAR, 0, {0}}, {"HELP", CMD_HELP, 0, {0}},
    {"SAVE", CMD_SAVE, 0, {0}}, {"DAC", CMD_DAC, 2, {19, 4095}},
    {"THRESH", CMD_THRESH, 3, {19, 32767, 32767}},
    {"POLARITY", CMD_POLARITY, 1, {1}}
};

void CommandParser_Init(void)
{
    line_length = 0;
    invalid_line = false;
}

void CommandParser_InvalidateLine(void)
{
    invalid_line = true;
}

static void SkipSpace(const char **cursor)
{
    while (**cursor == ' ' || **cursor == '\t') {
        ++*cursor;
    }
}

static bool ParseNumber(const char **cursor, uint16_t maximum, uint16_t *output)
{
    uint32_t value = 0;
    SkipSpace(cursor);
    if (**cursor < '0' || **cursor > '9') {
        return false;
    }
    do {
        unsigned digit = (unsigned)(**cursor - '0');
        if (value > (uint32_t)maximum / 10u || value * 10u + digit > maximum) {
            return false;
        }
        value = value * 10u + digit;
        ++*cursor;
    } while (**cursor >= '0' && **cursor <= '9');
    if (**cursor != '\0' && **cursor != ' ' && **cursor != '\t') {
        return false;
    }
    *output = (uint16_t)value;
    return true;
}

FwStatus CommandParser_Parse(const char *line, Command *command)
{
    if (line == NULL || command == NULL) {
        return FW_INVALID_ARGUMENT;
    }
    SkipSpace(&line);
    const char *end = line;
    while (*end != '\0' && *end != ' ' && *end != '\t') {
        ++end;
    }
    for (unsigned i = 0; i < sizeof(definitions) / sizeof(definitions[0]); ++i) {
        const CommandDefinition *definition = &definitions[i];
        if ((size_t)(end - line) != strlen(definition->name) ||
            strncmp(line, definition->name, (size_t)(end - line)) != 0) {
            continue;
        }
        Command parsed = {0};
        parsed.type = definition->type;
        const char *cursor = end;
        for (unsigned j = 0; j < definition->count; ++j) {
            if (!ParseNumber(&cursor, definition->limits[j], &parsed.args[j])) {
                return FW_INVALID_ARGUMENT;
            }
        }
        SkipSpace(&cursor);
        if (*cursor != '\0' || (parsed.type == CMD_THRESH && parsed.args[2] >= parsed.args[1])) {
            return FW_INVALID_ARGUMENT;
        }
        *command = parsed;
        return FW_OK;
    }
    return FW_INVALID_ARGUMENT;
}

ParserResult CommandParser_FeedByte(char byte, Command *command)
{
    if (byte == '\r' || byte == '\n') {
        ParserResult result = PARSER_NONE;
        if (invalid_line) {
            result = PARSER_ERROR;
        } else if (line_length != 0) {
            line_buffer[line_length] = '\0';
            const char *content = line_buffer;
            SkipSpace(&content);
            if (*content != '\0') {
                result = CommandParser_Parse(content, command) == FW_OK ? PARSER_READY : PARSER_ERROR;
            }
        }
        CommandParser_Init();
        return result;
    }
    if ((unsigned char)byte < 32 || (unsigned char)byte > 126) {
        invalid_line = true;
    } else if (line_length < sizeof(line_buffer) - 1u) {
        line_buffer[line_length++] = byte;
    } else {
        invalid_line = true;
    }
    return PARSER_NONE;
}
