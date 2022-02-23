#pragma once

#include <ntddk.h>
#include <wdf.h>
#include <initguid.h>

#include "WdfClassExtension.h"
#include "Trace.h"
#include "Public.h"


template <typename TData> struct EventItem {
    LIST_ENTRY List;
    TData Shared;
};

using BaseInfoItem = EventItem<BaseInfo>;
using ProcessCreateInfoItem = EventItem<ProcessCreateInfo>;
using ProcessDestroyInfoItem = EventItem<ProcessDestroyInfo>;


EXTERN_C_START

struct GlobalContext {
    LIST_ENTRY EventList;
    LONG64 ItemCount;
    FastMutex ListLock;
};

void ÑreateProcessNotifyRoutineEx(PEPROCESS Process, HANDLE ProcessId, PPS_CREATE_NOTIFY_INFO CreateInfo);
//void CreateThreadNotiftRoutine();

EXTERN_C_END