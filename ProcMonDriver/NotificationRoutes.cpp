#include "NotificationRoutes.h"
#include "NotificationRoutes.tmh"

GlobalContext g_GlobalContext;

void ÑreateProcessNotifyRoutineEx(PEPROCESS Process, HANDLE ProcessId, PPS_CREATE_NOTIFY_INFO CreateInfo)
{
	UNREFERENCED_PARAMETER(Process);

	TraceEvents(TRACE_LEVEL_INFORMATION, TRACE_DEVICE, "%!FUNC! Entry");

	size_t structSize = 0;
	size_t imagaFileNameSize = 0;
	size_t commandLineSize = 0;

	{
		AutoLock<FastMutex> lock(g_GlobalContext.ListLock);
		if (g_GlobalContext.ItemCount >= 100)
			return;
		// overflow...
	}

	if (CreateInfo)
	{
		structSize = sizeof(ProcessCreateInfoItem);

		// can use for ansii RtlxUnicodeStringToAnsiSizeCreateInfo
		if (CreateInfo->ImageFileName != NULL)
			imagaFileNameSize = CreateInfo->ImageFileName->Length;
		if (CreateInfo->CommandLine != NULL)
			commandLineSize = CreateInfo->CommandLine->Length;

		ProcessCreateInfoItem* processInfo = (ProcessCreateInfoItem*)ExAllocatePool(PagedPool, structSize + imagaFileNameSize + commandLineSize);
		if (processInfo == NULL) {
			TraceEvents(TRACE_LEVEL_INFORMATION, TRACE_DEVICE, "%!FUNC! [-] Memory allocation.");
			return;
		}

		processInfo->Shared.Type = EventType::ProcessCreate;
		processInfo->Shared.Size = sizeof(ProcessCreateInfo) + imagaFileNameSize + commandLineSize;
		processInfo->Shared.ProcessId = HandleToULong(ProcessId);
		processInfo->Shared.ParentProcessId = HandleToULong(CreateInfo->ParentProcessId);
		processInfo->Shared.ParentThreadId = HandleToULong(CreateInfo->CreatingThreadId.UniqueThread);

		processInfo->Shared.ImageFileNameOffset = sizeof(ProcessCreateInfo);
		processInfo->Shared.ImageFileNameLength = imagaFileNameSize;
		if (CreateInfo->ImageFileName != NULL)
			RtlCopyMemory(
				PCHAR(&processInfo->Shared) + processInfo->Shared.ImageFileNameOffset,
				CreateInfo->ImageFileName->Buffer,
				imagaFileNameSize
			);

		processInfo->Shared.CommandLineStringOffset = processInfo->Shared.ImageFileNameOffset + imagaFileNameSize;
		processInfo->Shared.CommandLineStringLength = commandLineSize;
		if (CreateInfo->CommandLine != NULL)
			RtlCopyMemory(
				PCHAR(&processInfo->Shared) + processInfo->Shared.CommandLineStringOffset,
				CreateInfo->CommandLine->Buffer,
				commandLineSize
			);

		AutoLock<FastMutex> lock(g_GlobalContext.ListLock);

		InsertHeadList(&g_GlobalContext.EventList, &processInfo->List);

		g_GlobalContext.ItemCount++;
	}
	else // Process destroy
	{
		structSize = sizeof(ProcessDestroyInfoItem);

		ProcessDestroyInfoItem* processInfo = (ProcessDestroyInfoItem*)ExAllocatePool(PagedPool, structSize);
		if (processInfo == NULL) {
			TraceEvents(TRACE_LEVEL_INFORMATION, TRACE_DEVICE, "%!FUNC! [-] Memory alloc.");
			return;
		}

		processInfo->Shared.Type = EventType::ProcessDestroy;
		processInfo->Shared.Size = sizeof(ProcessDestroyInfo);
		processInfo->Shared.ProcessId = HandleToULong(ProcessId);

		AutoLock<FastMutex> lock(g_GlobalContext.ListLock);

		InsertHeadList(&g_GlobalContext.EventList, &processInfo->List);

		g_GlobalContext.ItemCount++;
	}
}
