#pragma once

//
// Управляющий код начала процесса логгирования
// Входных аргументов нет.
// Выходных параметров нет.
//
#define IOCTL_CONTROL_MESSAGE_START CTL_CODE  (FILE_DEVICE_UNKNOWN, 0x801, METHOD_BUFFERED, FILE_ANY_ACCESS)


EXTERN_C_START

enum class EventType : char {
    Undefined,
    ProcessCreate,
    ProcessDestroy
};

//
// Базовая структура события
//
class BaseEventInfo {
public:
    size_t Size;
    EventType Type;
};

//
// Cтруктура события создания процесса 
// 
// Полный размер структуры считается следующим образом:
// Size = sizeof(ProcessCreateEventInfo) + ImageFileNameLength + CommandLineStringLength;
// 
// Строки:
//  1) ImageFileName
//  2) CommandLineString
//  - записываются в конце структуры, в соответствующей последовательности
//
class ProcessCreateEventInfo : public BaseEventInfo {
public:
    LONG64 ProcessId;
    LONG64 ParentProcessId;
    LONG64 ParentThreadId;

    // Строка имени процесса в формате Unicode
    LONG64 ImageFileNameOffset;
    LONG64 ImageFileNameLength;

    // Строка командных параметров процесса в формате Unicode
    LONG64 CommandLineStringOffset;
    LONG64 CommandLineStringLength;
};

//
// Cтруктура события удаления процесса 
//
class ProcessDestroyEventInfo : public BaseEventInfo {
public:
    LONG64 ProcessId;
};

EXTERN_C_END