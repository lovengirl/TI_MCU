#ifndef SCAN_CONTROLLER_H
#define SCAN_CONTROLLER_H
#include "sensor_types.h"
/* Hardware contract: main context only, except independent emergency cutoff. */
typedef struct {
    void (*tx_off)(void);
    void (*rx_off)(void);
    FwStatus (*select)(uint8_t tx, uint8_t rx);
    FwStatus (*set_level)(uint16_t code);
    void (*rx_on)(void);
    void (*tx_on)(void);
    FwStatus (*wait)(uint16_t us);
    FwStatus (*sample)(uint16_t period_us);
    void (*cancel)(void);
} ScanInterfaces;
void Scan_Init(const ScanInterfaces *interfaces);
FwStatus Scan_StartSingle(void);
FwStatus Scan_StartContinuous(void);
void Scan_RequestStop(void);
void Scan_EmergencyStop(void);
void Scan_EnterFault(uint32_t code);
FwStatus Scan_ClearFault(void);
void Scan_OnWaitComplete(void);
void Scan_OnSamples(const uint16_t *samples, size_t count);
void Scan_Process(void);
void Scan_GetStatus(AppStatus *status);
#endif
