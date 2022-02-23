#include "Queue.h"
#include "Device.h"
#include "Queue.tmh"

// Глобальный контекст драйвера
extern GlobalContext g_GlobalContext;

void EvtWdfIoQueueIoRead(WDFQUEUE Queue, WDFREQUEST Request, size_t Length)
{
	UNREFERENCED_PARAMETER(Queue);
	UNREFERENCED_PARAMETER(Request);
	UNREFERENCED_PARAMETER(Length);

	NTSTATUS status;
	UCHAR* transferBuffer;
	size_t transferBufferSize;
	size_t bufferWriteOffset = 0;

	static size_t BUFFER_MIN_SIZE = 128;


	TraceEvents(TRACE_LEVEL_INFORMATION, TRACE_DEVICE, "%!FUNC! Entry");

	status = WdfRequestRetrieveOutputBuffer(Request, BUFFER_MIN_SIZE, (PVOID*)(&transferBuffer), &transferBufferSize);
	if (!NT_SUCCESS(status)) {
		TraceEvents(TRACE_LEVEL_INFORMATION, TRACE_DEVICE, "%!FUNC! [-] WdfRequestRetrieveOutputBuffer return: %!STATUS!.", status);
		WdfRequestComplete(Request, STATUS_INVALID_PARAMETER);
		return;
	}

	AutoLock<FastMutex> lock(g_GlobalContext.ListLock);

	while (g_GlobalContext.ItemCount > 0)
	{
		// Критический случай, внезапно лист оказался пуст
		if (IsListEmpty(&g_GlobalContext.EventList)) {
			TraceEvents(TRACE_LEVEL_INFORMATION, TRACE_DEVICE, "%!FUNC! [Critical] entry list is empty!");
			WdfRequestComplete(Request, STATUS_INVALID_PARAMETER);
			return;
		}

		// Берем последний элемент
		const PLIST_ENTRY lastListItem = g_GlobalContext.EventList.Blink;
		const BaseEventInfoEntry* item = (BaseEventInfoEntry*)CONTAINING_RECORD(lastListItem, BaseEventInfoEntry, List);

		// Буфер заполнен
		if (item->SharedData.Size > transferBufferSize - bufferWriteOffset)
			goto __exit;

		// Удаляем элемент события из листа
		PLIST_ENTRY entry = RemoveTailList(&g_GlobalContext.EventList);
		g_GlobalContext.ItemCount--;
		
		item = (BaseEventInfoEntry*) CONTAINING_RECORD(entry, BaseEventInfoEntry, List);

		// Копируем структуру события во временный буфер
		RtlCopyMemory(transferBuffer + bufferWriteOffset, PVOID(&item->SharedData), item->SharedData.Size);
		bufferWriteOffset += item->SharedData.Size;

		ExFreePool(PVOID(item));
	}

	lock.free();

__exit:
	WdfRequestCompleteWithInformation(Request, STATUS_SUCCESS, bufferWriteOffset);
}


void EvtWdfIoQueueDeviceControl(WDFQUEUE Queue, WDFREQUEST Request, size_t OutputBufferLength, size_t InputBufferLength, ULONG IoControlCode)
{
	UNREFERENCED_PARAMETER(Request);
	UNREFERENCED_PARAMETER(OutputBufferLength);
	UNREFERENCED_PARAMETER(InputBufferLength);

	const WDFDEVICE deviceObject = WdfIoQueueGetDevice(Queue);
	PCONTROL_DEVICE_EXTENSION deviceExtension;

	TraceEvents(TRACE_LEVEL_INFORMATION, TRACE_DEVICE, "%!FUNC! Entry");


	// Управляющий код запуска процесса логгирования
	if (IOCTL_CONTROL_MESSAGE_START == IoControlCode)
	{
		deviceExtension = GetControlDeviceExtension(deviceObject);

		if (deviceExtension->notificationLevel == CONTROL_DEVICE_EXTENSION::NOTIFI_LEVELS::NOTHING_LOG)
		{
			deviceExtension->notificationLevel = CONTROL_DEVICE_EXTENSION::NOTIFI_LEVELS::PROCESS_LOG;
			PsSetCreateProcessNotifyRoutineEx(СreateProcessNotifyRoutineEx, FALSE);
		}
	}

	// Другие варианты для перехвата:
	//PsSetCreateThreadNotifyRoutineEx
	//PsRemoveCreateThreadNotifyRoutine 
	//PsSetLoadImageNotifyRoutineEx 
	//PsSetLoadImageNotifyRoutineEx 

	WdfRequestComplete(Request, STATUS_SUCCESS);
}