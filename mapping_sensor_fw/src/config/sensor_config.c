#include "sensor_config.h"
#include <string.h>

static SensorConfig active_config;
static SensorConfig pending_config;
static bool has_pending;
static uint32_t config_version;

void Config_LoadDefaults(SensorConfig *config)
{
    memset(config, 0, sizeof(*config));
    for (unsigned i = 0; i < SENSOR_CHANNELS; ++i) {
        config->dac[i] = 512;
        config->gain_q12[i] = 4096;
        config->enter[i] = 500;
        config->leave[i] = 400;
    }
    config->select_us = 50;
    config->light_us = 50;
    config->sample_us = 10;
    config->confirm = 2;
    config->polarity = 1;
}

void Config_Init(void)
{
    Config_LoadDefaults(&active_config);
    has_pending = false;
    config_version = 1;
}

bool Config_Validate(const SensorConfig *config)
{
    if (config == NULL || config->select_us < 5 || config->select_us > 10000 ||
        config->light_us < 5 || config->light_us > 10000 ||
        config->sample_us < 10 || config->sample_us > 1000 ||
        config->confirm < 1 || config->confirm > 20 || config->polarity > 1) {
        return false;
    }
    for (unsigned i = 0; i < SENSOR_CHANNELS; ++i) {
        if (config->dac[i] > 4095 || config->gain_q12[i] == 0 ||
            config->gain_q12[i] > 16384 || config->leave[i] >= config->enter[i]) {
            return false;
        }
    }
    return true;
}

FwStatus Config_Stage(const SensorConfig *candidate)
{
    if (!Config_Validate(candidate)) {
        return FW_INVALID_ARGUMENT;
    }
    pending_config = *candidate;
    has_pending = true;
    return FW_OK;
}

bool Config_ApplyAtFrameBoundary(void)
{
    if (!has_pending) {
        return false;
    }
    active_config = pending_config;
    has_pending = false;
    ++config_version;
    return true;
}

void Config_GetSnapshot(SensorConfig *config, uint32_t *version)
{
    *config = active_config;
    *version = config_version;
}

void Config_GetEditable(SensorConfig *config)
{
    *config = has_pending ? pending_config : active_config;
}

uint32_t Config_GetVersion(void)
{
    return config_version;
}
