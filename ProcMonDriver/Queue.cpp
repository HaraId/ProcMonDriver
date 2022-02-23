#include "Queue.h"
#include "Device.h"
#include "Queue.tmh"

extern GlobalContext g_GlobalContext;

void EvtWdfIoQueueIoRead(WDFQUEUE Queue, WDFREQUEST Request, size_t Length)
{
	UNREFERENCED_PARAMETER(Queue);
	UNREFERENCED_PARAMETER(Request);
	UNREFERENCED_PARAMETER(Length);

	NTSTATUS status;
	UCHAR* buffer;
	size_t bufferSize;
	size_t bufferWriteOffset = 0;

	const size_t BUFFER_MIN_SIZE = 128;

	TraceEvents(TRACE_LEVEL_INFORMATION, TRACE_DEVICE, "%!FUNC! Entry");

	status = WdfRequestRetrieveOutputBuffer(Request, BUFFER_MIN_SIZE, (PVOID*)(&buffer), &bufferSize);
	if (!NT_SUCCESS(status)) {
		TraceEvents(TRACE_LEVEL_INFORMATION, TRACE_DEVICE, "%!FUNC! [-] entry list is empty!");
		WdfRequestComplete(Request, STATUS_INVALID_PARAMETER);
		return;
	}

	AutoLock<FastMutex>(g_GlobalContext.ListLock);

	while (g_GlobalContext.ItemCount > 0)
	{
		if (IsListEmpty(&g_GlobalContext.EventList)) {
			TraceEvents(TRACE_LEVEL_INFORMATION, TRACE_DEVICE, "%!FUNC! [-] entry list is empty!");
			WdfRequestComplete(Request, STATUS_INVALID_PARAMETER);
			return;
		}

		const BaseInfoItem* item = (BaseInfoItem*)g_GlobalContext.EventList.Blink;

		if (item->Shared.Size > bufferSize - bufferWriteOffset)
			goto __exit;

		PLIST_ENTRY entry = RemoveTailList(&g_GlobalContext.EventList);
		g_GlobalContext.ItemCount--;
		
		item = (BaseInfoItem*) CONTAINING_RECORD(entry, BaseInfoItem, List);

		RtlCopyMemory(buffer + bufferWriteOffset, PVOID(&item->Shared), item->Shared.Size);
		bufferWriteOffset += item->Shared.Size;

		ExFreePool(PVOID(item));
	}

__exit:
	WdfRequestCompleteWithInformation(Request, STATUS_SUCCESS, bufferWriteOffset);
}

void EvtWdfIoQueueIoWrite(WDFQUEUE Queue, WDFREQUEST Request, size_t Length)
{
	UNREFERENCED_PARAMETER(Queue);
	UNREFERENCED_PARAMETER(Request);
	UNREFERENCED_PARAMETER(Length);

	TraceEvents(TRACE_LEVEL_INFORMATION, TRACE_DEVICE, "%!FUNC! Entry");

	WdfRequestComplete(Request, STATUS_SUCCESS);
}

void EvtWdfIoQueueDeviceControl(WDFQUEUE Queue, WDFREQUEST Request, size_t OutputBufferLength, size_t InputBufferLength, ULONG IoControlCode)
{
	UNREFERENCED_PARAMETER(Queue);
	UNREFERENCED_PARAMETER(Request);
	UNREFERENCED_PARAMETER(OutputBufferLength);
	UNREFERENCED_PARAMETER(InputBufferLength);
	UNREFERENCED_PARAMETER(IoControlCode);

	PAGED_CODE();

	TraceEvents(TRACE_LEVEL_INFORMATION, TRACE_DEVICE, "%!FUNC! Entry");

	/*if (!InputBufferLength)
	{
		WdfRequestComplete(Request, STATUS_INVALID_PARAMETER);
		return;
	}*/

	switch (IoControlCode)
	{
	case IOCTL_CONTROL_MESSAGE:
		PCONTROL_DEVICE_EXTENSION deviceExtension = GetControlDeviceData(WdfIoQueueGetDevice(Queue));

		if (deviceExtension->notificationLevel == CONTROL_DEVICE_EXTENSION::NOTIFI_LEVELS::PROCESS_LOG)
			break;

		deviceExtension->notificationLevel = CONTROL_DEVICE_EXTENSION::NOTIFI_LEVELS::PROCESS_LOG;

		PsSetCreateProcessNotifyRoutineEx(ÑreateProcessNotifyRoutineEx, FALSE);
		break;
	}
	//PsSetCreateThreadNotifyRoutineEx
	//PsRemoveCreateThreadNotifyRoutine 
	//PsSetLoadImageNotifyRoutineEx 
	//PsSetLoadImageNotifyRoutineEx 

	WdfRequestComplete(Request, STATUS_SUCCESS);
}