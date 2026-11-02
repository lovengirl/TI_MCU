#include "watchdog_service.h"
#include "measurement_timer.h"
#include "ti_msp_dl_config.h"
static uint32_t last_feed;

void WatchdogService_Init(void)
{
    last_feed = MeasurementTimer_Millis();
}

void WatchdogService_Process(void)
{
    /* Called only after all foreground tasks returned. Tick must also advance. */
    uint32_t now = MeasurementTimer_Millis();
    if ((uint32_t)(now - last_feed) >= 100u) {
        DL_WWDT_restart(WWDT0);
        last_feed = now;
    }
}
