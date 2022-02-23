#pragma once

#include <ntddk.h>
#include <wdf.h>
#include <initguid.h>

#include "Trace.h"
#include "HookRoutine.h"
#include "ThreadLockClasses.h"

EXTERN_C_START

// IRP_MJ_READ
EVT_WDF_IO_QUEUE_IO_READ EvtWdfIoQueueIoRead;

// IRP_MJ_DEVICE_CONTROL
EVT_WDF_IO_QUEUE_IO_DEVICE_CONTROL EvtWdfIoQueueDeviceControl;

EXTERN_C_END
