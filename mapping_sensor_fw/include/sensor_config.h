#ifndef SENSOR_CONFIG_H
#define SENSOR_CONFIG_H
#include "sensor_types.h"
void Config_Init(void);
void Config_LoadDefaults(SensorConfig *config);
bool Config_Validate(const SensorConfig *config);
FwStatus Config_Stage(const SensorConfig *candidate);
bool Config_ApplyAtFrameBoundary(void);
void Config_GetSnapshot(SensorConfig *config, uint32_t *version);
void Config_GetEditable(SensorConfig *config);
uint32_t Config_GetVersion(void);
#endif
