#ifndef DIAGNOSTICS_H
#define DIAGNOSTICS_H
#include "sensor_types.h"
typedef struct { uint32_t fault, total_faults, context; } DiagnosticStatus;
void Diagnostics_Init(void);
void Diagnostics_ReportFault(uint32_t code, uint32_t context);
void Diagnostics_GetSnapshot(DiagnosticStatus *status);
void Diagnostics_ClearFault(void);
#endif
