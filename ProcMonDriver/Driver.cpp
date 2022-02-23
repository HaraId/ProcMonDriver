#include "driver.h"
#include "driver.tmh"


#ifdef ALLOC_PRAGMA
#pragma alloc_text (INIT, DriverEntry)
#pragma alloc_text (PAGE, EvtWdfDriverUnload)
#pragma alloc_text (PAGE, EvtWdfDriverObjectContextCleanup)
#pragma alloc_text (PAGE, EvtWdfDriverObjectContextDestroy)
#endif


NTSTATUS
DriverEntry(
    _In_ PDRIVER_OBJECT  DriverObject,
    _In_ PUNICODE_STRING RegistryPath
    )

{
    NTSTATUS status;
    WDFDRIVER wdfDriver;
    WDF_OBJECT_ATTRIBUTES driverAttribute;
    WDF_DRIVER_CONFIG driverConfig;
    
    //
    // Initialize WPP Tracing
    //
    WPP_INIT_TRACING(DriverObject, RegistryPath);
    TraceEvents(TRACE_LEVEL_INFORMATION, TRACE_DRIVER, "%!FUNC! Entry");


    WDF_DRIVER_CONFIG_INIT(
        &driverConfig, 
        WDF_NO_EVENT_CALLBACK   // No DeviceAdd callback function
    );

    driverConfig.EvtDriverUnload = EvtWdfDriverUnload;

    driverConfig.DriverInitFlags |= WdfDriverInitNonPnpDriver;

    WDF_OBJECT_ATTRIBUTES_INIT(&driverAttribute);

    // The framework calls the callback function when either the framework or a driver attempts to delete object.
    // can decrement reference to object
    driverAttribute.EvtCleanupCallback = EvtWdfDriverObjectContextCleanup;

    // The framework calls the EvtCleanupCallback function first.
    // The framework calls the EvtDestroyCallback callback function after the object's reference count has been decremented to zero.
    driverAttribute.EvtDestroyCallback = EvtWdfDriverObjectContextDestroy;

    status = WdfDriverCreate(DriverObject,
        RegistryPath,
        &driverAttribute,
        &driverConfig,
        &wdfDriver
    );
    
    if (!NT_SUCCESS(status)) {
        TraceEvents(TRACE_LEVEL_INFORMATION, TRACE_DRIVER, "%!FUNC! Driver object create error.");
        WPP_CLEANUP(DriverObject);
        return status;
    }


    status = CreateControlDevice(wdfDriver);
    if (!NT_SUCCESS(status)) {
        TraceEvents(TRACE_LEVEL_INFORMATION, TRACE_DRIVER, "%!FUNC! Device object create error.");
        WPP_CLEANUP(DriverObject);
        return status;
    }

    TraceEvents(TRACE_LEVEL_INFORMATION, TRACE_DRIVER, "%!FUNC! Exit");

    return status;
}


VOID EvtWdfDriverUnload(WDFDRIVER Driver)
{
    UNREFERENCED_PARAMETER(Driver);
    PAGED_CODE();
}


VOID EvtWdfDriverObjectContextCleanup(WDFOBJECT Object)
{
    UNREFERENCED_PARAMETER(Object);

    PAGED_CODE();
    TraceEvents(TRACE_LEVEL_INFORMATION, TRACE_DRIVER, "%!FUNC! Entry. [+] Driver attribute context cleanup");
}


VOID EvtWdfDriverObjectContextDestroy(WDFOBJECT Object)
{
    UNREFERENCED_PARAMETER(Object);

    PAGED_CODE();
    TraceEvents(TRACE_LEVEL_INFORMATION, TRACE_DRIVER, "%!FUNC! Entry. [+] Driver attribute context destroy");

    WPP_CLEANUP(WdfDriverWdmGetDriverObject((WDFDRIVER)Object));
}