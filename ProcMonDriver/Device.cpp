#include "Device.h"
#include "Device.tmh"

// Глобальны контекст драйвера
GlobalContext g_GlobalContext;


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
    // освоболить занимаемое структурой место 
    deviceInit = WdfControlDeviceInitAllocate(Driver, &SDDL_DEVOBJ_SYS_ALL_ADM_RWX_WORLD_RW_RES_R); //ADD to linker: $(DDK_LIB_PATH)\wdmsec.lib

    if (deviceInit == NULL) {
        TraceEvents(TRACE_LEVEL_INFORMATION, TRACE_DEVICE, "%!FUNC! Entry. [-] Critical error for DeviceInit allocation");
        status = STATUS_INSUFFICIENT_RESOURCES;
        return status;
    }

    // Включаем буферизацию для всех запросов к девайсу управления
    WdfDeviceInitSetIoType(deviceInit, WdfDeviceIoBuffered);


    //
    // Регистрация обратных вызовов для действий над файлом объекта девайса (открытие/закрытие)
    //
    WDF_OBJECT_ATTRIBUTES_INIT(&fileAttribute);
    // Методика обеспечения синхронного вызова функций, в данном случае синхронизация нам не нужна
    fileAttribute.SynchronizationScope = WdfSynchronizationScopeNone;

    WDF_FILEOBJECT_CONFIG_INIT(&fileConfig,
        EvtWdfDeviceFileCreate,   // device file create 
        EvtWdfFileClose,   // device file close
        WDF_NO_EVENT_CALLBACK);  // device file cleanup

    WdfDeviceInitSetFileObjectConfig(deviceInit, &fileConfig, &fileAttribute);
    

    // В один момент времени к устройству может иметь доступ только один клиент
    WdfDeviceInitSetExclusive(deviceInit, TRUE);


    // У всех управляющих устройств должно быть задано имя, иначе мы не определим для него символистическую ссылку
    status = WdfDeviceInitAssignName(deviceInit, &deviceName);

    if (!NT_SUCCESS(status)) {
        TraceEvents(TRACE_LEVEL_INFORMATION, TRACE_DEVICE, "WdfDeviceInitAssignName failed %!STATUS!", status);
        WdfDeviceInitFree(deviceInit);
        return status;
    }


    //
    // Инициализация и создание самого объекта управляющего устройства 
    //
    WDF_OBJECT_ATTRIBUTES_INIT_CONTEXT_TYPE(&deviceAttribute, CONTROL_DEVICE_EXTENSION);

    deviceAttribute.EvtCleanupCallback = EvtWdfDeviceContextCleanup;

    // После того как драйвер выполнит WdfDeviceCreate, он больше не должен обащаться к объекту deviceInit.
    status = WdfDeviceCreate(&deviceInit, &deviceAttribute, &device);
    if (!NT_SUCCESS(status)) {
        TraceEvents(TRACE_LEVEL_INFORMATION, TRACE_DEVICE, "WdfDeviceCreate failed %!STATUS!", status);
        return status;
    }

    PCONTROL_DEVICE_EXTENSION deviceExtension = GetControlDeviceExtension(device);
    deviceExtension->notificationLevel = CONTROL_DEVICE_EXTENSION::NOTIFI_LEVELS::NOTHING_LOG;


    //
    // Создание стандартной очереди запросов для CDO
    //
    WDF_OBJECT_ATTRIBUTES_INIT(&queueAttribute);

    WDF_IO_QUEUE_CONFIG_INIT_DEFAULT_QUEUE(&queueConfig, WdfIoQueueDispatchSequential);
    queueConfig.EvtIoDeviceControl = EvtWdfIoQueueDeviceControl;
    queueConfig.EvtIoRead = EvtWdfIoQueueIoRead;

    status = WdfIoQueueCreate(device, &queueConfig, &queueAttribute, &queue);
    if (!NT_SUCCESS(status)) {
        TraceEvents(TRACE_LEVEL_INFORMATION, TRACE_DEVICE, "WdfIoQueueCreate failed %!STATUS!", status);
        return status;
    }
    
    // Создание ссылки на устройство
    status = WdfDeviceCreateSymbolicLink(device, &deviceSymbolicName);
    if(!NT_SUCCESS(status)) {
        TraceEvents(TRACE_LEVEL_INFORMATION, TRACE_DEVICE, "WdfDeviceCreate failed %!STATUS!", status);
        return status;
    }


    // Завершение создания устройства управления
    WdfControlFinishInitializing(device);

    // Инициализация глобального контекста 
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

    PCONTROL_DEVICE_EXTENSION deviceExtension = GetControlDeviceExtension(WdfFileObjectGetDevice(FileObject));

    // останавливаем логирование
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

    PCONTROL_DEVICE_EXTENSION deviceExtension = GetControlDeviceExtension(Object);

    // останавливаем логирование
    if (deviceExtension->notificationLevel == CONTROL_DEVICE_EXTENSION::NOTIFI_LEVELS::PROCESS_LOG)
    {
        PsSetCreateProcessNotifyRoutineEx(СreateProcessNotifyRoutineEx, TRUE);
        deviceExtension->notificationLevel = CONTROL_DEVICE_EXTENSION::NOTIFI_LEVELS::NOTHING_LOG;
    }


    AutoLock<FastMutex> lock(g_GlobalContext.ListLock);
    
    // Удаляем все эдементы в списке событий
    while (g_GlobalContext.ItemCount > 0 && !IsListEmpty(&g_GlobalContext.EventList))
    {
        const PLIST_ENTRY entry = RemoveTailList(&g_GlobalContext.EventList);
        const BaseEventInfoEntry* item = (BaseEventInfoEntry*)CONTAINING_RECORD(entry, BaseEventInfoEntry, List);

        ExFreePool(PVOID(item));
    }
}