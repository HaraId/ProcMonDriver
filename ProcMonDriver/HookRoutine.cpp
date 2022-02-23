#include "HookRoutine.h"
#include "HookRoutine.tmh"

#include "Device.h"


// Глобальны контекст 
extern GlobalContext g_GlobalContext;


void СreateProcessNotifyRoutineEx(PEPROCESS Process, HANDLE ProcessId, PPS_CREATE_NOTIFY_INFO CreateInfo)
{
	UNREFERENCED_PARAMETER(Process);

	TraceEvents(TRACE_LEVEL_INFORMATION, TRACE_DEVICE, "%!FUNC! Entry");

	size_t imagaFileNameSize = 0;
	size_t commandLineSize = 0;
	const size_t MAX_EVENT_ENTRY_COUNT = 512;

	//
	// Ставим ограничение на количество событий, которые одновременно может зранить драйвер
	// 
	AutoLock<FastMutex> lock(g_GlobalContext.ListLock);

	if (g_GlobalContext.ItemCount >= MAX_EVENT_ENTRY_COUNT)
		return;

	lock.free();


	//
	// Событие создания нового процесса
	//
	if (CreateInfo)
	{
		if (CreateInfo->ImageFileName != NULL)
			imagaFileNameSize = CreateInfo->ImageFileName->Length;
		if (CreateInfo->CommandLine != NULL)
			commandLineSize = CreateInfo->CommandLine->Length;

		ProcessCreateEventInfoEntry* eventItem = (ProcessCreateEventInfoEntry*)ExAllocatePool(
			PagedPool, 
			sizeof(ProcessCreateEventInfoEntry) + imagaFileNameSize + commandLineSize
		);
		if (eventItem == NULL) {
			TraceEvents(TRACE_LEVEL_INFORMATION, TRACE_DEVICE, "%!FUNC! [-] Memory allocation.");
			return;
		}

		eventItem->SharedData.Type = EventType::ProcessCreate;

		// В полном размере структуры не учитывается поле LIST_ENTRY
		eventItem->SharedData.Size = sizeof(ProcessCreateEventInfo) + imagaFileNameSize + commandLineSize;
		
		eventItem->SharedData.ProcessId = HandleToULong(ProcessId);
		eventItem->SharedData.ParentProcessId = HandleToULong(CreateInfo->ParentProcessId);
		eventItem->SharedData.ParentThreadId = HandleToULong(CreateInfo->CreatingThreadId.UniqueThread);

		// Сериализация строки названия образа
		eventItem->SharedData.ImageFileNameOffset = sizeof(ProcessCreateEventInfo);
		eventItem->SharedData.ImageFileNameLength = imagaFileNameSize;
		if (CreateInfo->ImageFileName != NULL)
			RtlCopyMemory(
				PCHAR(&eventItem->SharedData) + eventItem->SharedData.ImageFileNameOffset,
				CreateInfo->ImageFileName->Buffer,
				imagaFileNameSize
			);

		// Сериализация строки параметров командной строки
		eventItem->SharedData.CommandLineStringOffset = eventItem->SharedData.ImageFileNameOffset + imagaFileNameSize;
		eventItem->SharedData.CommandLineStringLength = commandLineSize;
		if (CreateInfo->CommandLine != NULL)
			RtlCopyMemory(
				PCHAR(&eventItem->SharedData) + eventItem->SharedData.CommandLineStringOffset,
				CreateInfo->CommandLine->Buffer,
				commandLineSize
			);


		AutoLock<FastMutex> lock_inc(g_GlobalContext.ListLock);
		InsertHeadList(&g_GlobalContext.EventList, &eventItem->List);
		g_GlobalContext.ItemCount++;
	}

	//
	// Событие удаления процесса
	//
	else 
	{
		ProcessDestroyEventInfoEntry* eventItem = (ProcessDestroyEventInfoEntry*)ExAllocatePool(
			PagedPool, 
			sizeof(ProcessDestroyEventInfoEntry)
		);
		if (eventItem == NULL) {
			TraceEvents(TRACE_LEVEL_INFORMATION, TRACE_DEVICE, "%!FUNC! [-] Memory alloc.");
			return;
		}

		eventItem->SharedData.Type = EventType::ProcessDestroy;
		eventItem->SharedData.Size = sizeof(ProcessDestroyEventInfo);
		eventItem->SharedData.ProcessId = HandleToULong(ProcessId);


		AutoLock<FastMutex> lock_inc(g_GlobalContext.ListLock);
		InsertHeadList(&g_GlobalContext.EventList, &eventItem->List);
		g_GlobalContext.ItemCount++;
	}

}
