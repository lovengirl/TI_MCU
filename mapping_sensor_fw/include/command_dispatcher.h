#ifndef COMMAND_DISPATCHER_H
#define COMMAND_DISPATCHER_H
#include "command_parser.h"
#include "response_formatter.h"
void CommandDispatcher_Execute(const Command *command, const AppStatus *status,
                               const SensorFrame *latest, CommandReply *reply);
#endif
