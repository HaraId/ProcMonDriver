#pragma once

#include <ntddk.h>
#include <wdf.h>
#include <initguid.h>

#include "Trace.h"
#include "NotificationRoutes.h"
#include "WdfClassExtension.h"

EXTERN_C_START

typedef struct _DEVICE_QUEUE_EXTENSION {
    PVOID data;
}DEVICE_QUEUE_EXTENSION, *PDEVICE_QUEUE_EXTENSION;

WDF_DECLARE_CONTEXT_TYPE_WITH_NAME(DEVICE_QUEUE_EXTENSION, GetQueueData);

EVT_WDF_IO_QUEUE_IO_READ EvtWdfIoQueueIoRead;
EVT_WDF_IO_QUEUE_IO_WRITE EvtWdfIoQueueIoWrite;
EVT_WDF_IO_QUEUE_IO_DEVICE_CONTROL EvtWdfIoQueueDeviceControl;

EXTERN_C_END
