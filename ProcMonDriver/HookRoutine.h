#pragma once

#include <ntddk.h>
#include <wdf.h>

#include "ThreadLockClasses.h"
#include "Trace.h"


EXTERN_C_START

/*
	FUNC: ÑreateProcessNotifyRoutineEx() - A callback routine implemented by a driver to notify the caller when a process is created or exits.
	
	[_Inout_] Process
		A pointer to the EPROCESS structure that represents the process. Drivers can use the PsGetCurrentProcess and ObReferenceObjectByHandle routines to obtain a pointer to the EPROCESS structure for a process.

	[in] ProcessId
		The process ID of the process.

	[in, out, optional] CreateInfo
		A pointer to a PS_CREATE_NOTIFY_INFO structure that contains information about the new process. If this parameter is NULL, the specified process is exiting.
	
	Return value:
		None

	IRQL: PASSIVE_LEVEL
*/
void ÑreateProcessNotifyRoutineEx(PEPROCESS Process, HANDLE ProcessId, PPS_CREATE_NOTIFY_INFO CreateInfo);

EXTERN_C_END