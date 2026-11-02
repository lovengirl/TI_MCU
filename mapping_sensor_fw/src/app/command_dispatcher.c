#include "command_dispatcher.h"
#include "scan_controller.h"
#include "sensor_config.h"
#include <string.h>

static FwStatus UpdateConfig(const Command *command)
{
    SensorConfig candidate;
    Config_GetEditable(&candidate);
    unsigned channel = command->args[0];
    if (command->type == CMD_DAC) {
        if (channel >= SENSOR_CHANNELS) {
            return FW_INVALID_ARGUMENT;
        }
        candidate.dac[channel] = command->args[1];
    } else if (command->type == CMD_THRESH) {
        if (channel >= SENSOR_CHANNELS || command->args[1] > 32767 || command->args[2] > 32767) {
            return FW_INVALID_ARGUMENT;
        }
        candidate.enter[channel] = (int16_t)command->args[1];
        candidate.leave[channel] = (int16_t)command->args[2];
    } else {
        if (command->args[0] > 1) {
            return FW_INVALID_ARGUMENT;
        }
        candidate.polarity = (uint8_t)command->args[0];
    }
    return Config_Stage(&candidate);
}

void CommandDispatcher_Execute(const Command *command, const AppStatus *status,
                               const SensorFrame *latest, CommandReply *reply)
{
    memset(reply, 0, sizeof(*reply));
    reply->type = REPLY_ACK;
    FwStatus result = FW_INVALID_ARGUMENT;
    switch (command->type) {
    case CMD_STATUS:
        reply->type = REPLY_STATUS;
        reply->status = *status;
        return;
    case CMD_RESULT:
        reply->type = latest == NULL ? REPLY_NO_DATA : REPLY_FRAME;
        if (latest != NULL) {
            reply->frame = *latest;
        }
        return;
    case CMD_HELP:
        reply->type = REPLY_HELP;
        return;
    case CMD_SAVE:
        reply->type = REPLY_SAVE;
        return;
    case CMD_SINGLE:
        result = Scan_StartSingle();
        break;
    case CMD_START:
        result = Scan_StartContinuous();
        break;
    case CMD_STOP:
        Scan_RequestStop();
        result = FW_OK;
        break;
    case CMD_ESTOP:
        Scan_EmergencyStop();
        result = FW_OK;
        break;
    case CMD_CLEAR:
        result = Scan_ClearFault();
        break;
    case CMD_DAC:
    case CMD_THRESH:
    case CMD_POLARITY:
        result = UpdateConfig(command);
        Scan_Process();
        break;
    }
    reply->ok = result == FW_OK;
}
