#pragma once

#include "Queue.h"
#include "Public.h"


#define DEVICE_NAME L"\\Device\\ProcMon"
#define DEVICE_SYMBOLIC_NAME L"\\DosDevices\\ProcMon"



template <typename TData> struct EventListEntry {
    LIST_ENTRY List;
    TData SharedData;
};

//
//
//
using BaseEventInfoEntry = EventListEntry<BaseEventInfo>;
using ProcessCreateEventInfoEntry = EventListEntry<ProcessCreateEventInfo>;
using ProcessDestroyEventInfoEntry = EventListEntry<ProcessDestroyEventInfo>;


EXTERN_C_START

struct GlobalContext {
    LIST_ENTRY EventList;
    LONG64 ItemCount;
    FastMutex ListLock;
};

typedef struct _CONTROL_DEVICE_EXTENSION 
{
    enum class NOTIFI_LEVELS : BYTE {
        NOTHING_LOG = (0),
        PROCESS_LOG = (1)
    };

    NOTIFI_LEVELS notificationLevel;

} CONTROL_DEVICE_EXTENSION, *PCONTROL_DEVICE_EXTENSION;

WDF_DECLARE_CONTEXT_TYPE_WITH_NAME(CONTROL_DEVICE_EXTENSION, GetControlDeviceExtension)


NTSTATUS CreateControlDevice(WDFDRIVER Driver);
EVT_WDF_OBJECT_CONTEXT_CLEANUP EvtWdfDeviceContextCleanup;


EVT_WDF_DEVICE_FILE_CREATE EvtWdfDeviceFileCreate;
EVT_WDF_FILE_CLOSE EvtWdfFileClose;


EXTERN_C_END
