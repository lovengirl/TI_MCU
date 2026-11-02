#include "diagnostics.h"
#include <string.h>
static DiagnosticStatus status;

void Diagnostics_Init(void)
{
    memset(&status, 0, sizeof(status));
}

void Diagnostics_ReportFault(uint32_t code, uint32_t context)
{
    if (status.fault == 0) {
        status.fault = code;
        status.context = context;
    }
    ++status.total_faults;
}

void Diagnostics_GetSnapshot(DiagnosticStatus *output)
{
    *output = status;
}

void Diagnostics_ClearFault(void)
{
    status.fault = 0;
    status.context = 0;
}
