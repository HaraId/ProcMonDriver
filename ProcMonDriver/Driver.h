#pragma once


#include "Trace.h"
#include "Device.h"

EXTERN_C_START

DRIVER_INITIALIZE DriverEntry;
EVT_WDF_DRIVER_UNLOAD EvtWdfDriverUnload;

EVT_WDF_OBJECT_CONTEXT_CLEANUP EvtWdfDriverObjectContextCleanup;
EVT_WDF_OBJECT_CONTEXT_DESTROY EvtWdfDriverObjectContextDestroy;

EXTERN_C_END
