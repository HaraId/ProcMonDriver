/*++

Module Name:

    device.c - Device handling events for example driver.

Abstract:

   This file contains the device entry points and callbacks.
    
Environment:

    Kernel-mode Driver Framework

--*/

#include "Device.h"
#include "Device.tmh"

extern GlobalContext g_GlobalContext;

#ifdef ALLOC_PRAGMA
#pragma alloc_text (PAGE, CreateControlDevice)
#pragma alloc_text (PAGE, EvtWdfDeviceFileCreate)
#pragma alloc_text (PAGE, EvtWdfFileClose)
#endif

NTSTATUS CreateControlDevice(WDFDRIVER Driver)
{
    NTSTATUS status = STATUS_SUCCESS;
    PWDFDEVICE_INIT deviceInit;
    WDFDEVICE device;

    WDF_FILEOBJECT_CONFIG fileConfig;
    WDF_OBJECT_ATTRIBUTES fileAttribute;

    WDF_OBJECT_ATTRIBUTES deviceAttribute;

    WDFQUEUE queue;
    WDF_OBJECT_ATTRIBUTES queueAttribute;
    WDF_IO_QUEUE_CONFIG queueConfig;

    // Объявляем UNICODE строку имени устройства и имени ссылки на него
    DECLARE_CONST_UNICODE_STRING(deviceName, DEVICE_NAME);
    DECLARE_CONST_UNICODE_STRING(deviceSymbolicName, DEVICE_SYMBOLIC_NAME);
    
    // Выделяем структуру инициализации объекта девайса управления
    // В случае если до успешного завершения WdfCreateDevice произойдет ошибка, следует
    // Освоболить занимаемое структурой место 
    deviceInit = WdfControlDeviceInitAllocate(Driver, &SDDL_DEVOBJ_SYS_ALL_ADM_RWX_WORLD_RW_RES_R); //ADD to linker: $(DDK_LIB_PATH)\wdmsec.lib

    if (deviceInit == NULL) {
        TraceEvents(TRACE_LEVEL_INFORMATION, TRACE_DEVICE, "%!FUNC! Entry. [-] Critical error for DeviceInit allocation");
        status = STATUS_INSUFFICIENT_RESOURCES;
        return status;
    }


    WdfDeviceInitSetIoType(deviceInit, WdfDeviceIoBuffered);


    {
        WDF_OBJECT_ATTRIBUTES_INIT(&fileAttribute);
        // Методика обеспечения синхронного вызова функций, в данном случае без синхронизации 
        fileAttribute.SynchronizationScope = WdfSynchronizationScopeNone;

        WDF_FILEOBJECT_CONFIG_INIT(&fileConfig,
            EvtWdfDeviceFileCreate,   // device file create 
            EvtWdfFileClose,   // device file close
            WDF_NO_EVENT_CALLBACK);  // device file cleanup

        WdfDeviceInitSetFileObjectConfig(deviceInit, &fileConfig, &fileAttribute);
    }
    
    // В момент времени к устройству может иметь доступ только один клиент
    WdfDeviceInitSetExclusive(deviceInit, TRUE);

    // У всех управляющих устройств должно быть задано имя, иначе мы не зададим на него символистическую ссылку
    status = WdfDeviceInitAssignName(deviceInit, &deviceName);

    if (!NT_SUCCESS(status)) {
        TraceEvents(TRACE_LEVEL_INFORMATION, TRACE_DEVICE, "WdfDeviceInitAssignName failed %!STATUS!", status);
        WdfDeviceInitFree(deviceInit);
        return status;
    }


    WDF_OBJECT_ATTRIBUTES_INIT_CONTEXT_TYPE(&deviceAttribute, CONTROL_DEVICE_EXTENSION);

    deviceAttribute.EvtCleanupCallback = EvtWdfDeviceContextCleanup;

    // After the driver calls WdfDeviceCreate, it can no longer access the WDFDEVICE_INIT structure.
    status = WdfDeviceCreate(&deviceInit, &deviceAttribute, &device);
    if (!NT_SUCCESS(status)) {
        TraceEvents(TRACE_LEVEL_INFORMATION, TRACE_DEVICE, "WdfDeviceCreate failed %!STATUS!", status);
        return status;
    }

    PCONTROL_DEVICE_EXTENSION deviceExtension = GetControlDeviceData(device);
    deviceExtension->notificationLevel = CONTROL_DEVICE_EXTENSION::NOTIFI_LEVELS::NOTHING_LOG;

    {
        WDF_OBJECT_ATTRIBUTES_INIT_CONTEXT_TYPE(&queueAttribute, DEVICE_QUEUE_EXTENSION);
        
        WDF_IO_QUEUE_CONFIG_INIT_DEFAULT_QUEUE(&queueConfig, WdfIoQueueDispatchSequential);
        queueConfig.EvtIoDeviceControl = EvtWdfIoQueueDeviceControl;
        queueConfig.EvtIoRead = EvtWdfIoQueueIoRead;
        queueConfig.EvtIoWrite = EvtWdfIoQueueIoWrite;

        status = WdfIoQueueCreate(device, &queueConfig, &queueAttribute, &queue);
        if (!NT_SUCCESS(status)) {
            TraceEvents(TRACE_LEVEL_INFORMATION, TRACE_DEVICE, "WdfIoQueueCreate failed %!STATUS!", status);
            return status;
        }
    }
    
    status = WdfDeviceCreateSymbolicLink(device, &deviceSymbolicName);
    if(!NT_SUCCESS(status)) {
        TraceEvents(TRACE_LEVEL_INFORMATION, TRACE_DEVICE, "WdfDeviceCreate failed %!STATUS!", status);
        return status;
    }

    WdfControlFinishInitializing(device);


    g_GlobalContext.ListLock.Init();
    InitializeListHead(&g_GlobalContext.EventList);

    return status;
}


void EvtWdfDeviceFileCreate(WDFDEVICE Device, WDFREQUEST Request, WDFFILEOBJECT FileObject)
{
    UNREFERENCED_PARAMETER(Device);
    UNREFERENCED_PARAMETER(FileObject);

    TraceEvents(TRACE_LEVEL_INFORMATION, TRACE_DEVICE, "%!FUNC! Entry");

    // Success status for create file object
    WdfRequestComplete(Request, STATUS_SUCCESS);
}

void EvtWdfFileClose(WDFFILEOBJECT FileObject)
{
    TraceEvents(TRACE_LEVEL_INFORMATION, TRACE_DEVICE, "%!FUNC! Entry");

    PCONTROL_DEVICE_EXTENSION deviceExtension = GetControlDeviceData(WdfFileObjectGetDevice(FileObject));

    if (deviceExtension->notificationLevel == CONTROL_DEVICE_EXTENSION::NOTIFI_LEVELS::PROCESS_LOG)
    {
        PsSetCreateProcessNotifyRoutineEx(СreateProcessNotifyRoutineEx, TRUE);
        deviceExtension->notificationLevel = CONTROL_DEVICE_EXTENSION::NOTIFI_LEVELS::NOTHING_LOG;
    }
}

VOID EvtWdfDeviceContextCleanup(WDFOBJECT Object)
{
    UNREFERENCED_PARAMETER(Object);

    PAGED_CODE();
    TraceEvents(TRACE_LEVEL_INFORMATION, TRACE_DRIVER, "%!FUNC! Entry.");

    PCONTROL_DEVICE_EXTENSION deviceExtension = GetControlDeviceData(Object);

    if (deviceExtension->notificationLevel == CONTROL_DEVICE_EXTENSION::NOTIFI_LEVELS::PROCESS_LOG)
    {
        PsSetCreateProcessNotifyRoutineEx(СreateProcessNotifyRoutineEx, TRUE);
        deviceExtension->notificationLevel = CONTROL_DEVICE_EXTENSION::NOTIFI_LEVELS::NOTHING_LOG;
    }

    AutoLock<FastMutex> lock(g_GlobalContext.ListLock);

    while (g_GlobalContext.ItemCount > 0 && !IsListEmpty(&g_GlobalContext.EventList))
    {
        const PLIST_ENTRY entry = RemoveTailList(&g_GlobalContext.EventList);
        const BaseInfoItem* item = (BaseInfoItem*)CONTAINING_RECORD(entry, BaseInfoItem, List);

        ExFreePool(PVOID(item));
    }
}