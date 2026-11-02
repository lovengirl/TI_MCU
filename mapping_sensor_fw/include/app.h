#ifndef APP_H
#define APP_H
#include "sensor_types.h"
void App_Init(void);
void App_Process(void);
void App_GetStatus(AppStatus *status);
bool App_GetLatestFrame(SensorFrame *frame);
#endif
