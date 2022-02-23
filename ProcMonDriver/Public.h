#pragma once

//
// ����������� ��� ������ �������� ������������
// ������� ���������� ���.
// �������� ���������� ���.
//
#define IOCTL_CONTROL_MESSAGE_START CTL_CODE  (FILE_DEVICE_UNKNOWN, 0x801, METHOD_BUFFERED, FILE_ANY_ACCESS)


EXTERN_C_START

enum class EventType : char {
    Undefined,
    ProcessCreate,
    ProcessDestroy
};

//
// ������� ��������� �������
//
class BaseEventInfo {
public:
    size_t Size;
    EventType Type;
};

//
// C�������� ������� �������� �������� 
// 
// ������ ������ ��������� ��������� ��������� �������:
// Size = sizeof(ProcessCreateEventInfo) + ImageFileNameLength + CommandLineStringLength;
// 
// ������:
//  1) ImageFileName
//  2) CommandLineString
//  - ������������ � ����� ���������, � ��������������� ������������������
//
class ProcessCreateEventInfo : public BaseEventInfo {
public:
    LONG64 ProcessId;
    LONG64 ParentProcessId;
    LONG64 ParentThreadId;

    // ������ ����� �������� � ������� Unicode
    LONG64 ImageFileNameOffset;
    LONG64 ImageFileNameLength;

    // ������ ��������� ���������� �������� � ������� Unicode
    LONG64 CommandLineStringOffset;
    LONG64 CommandLineStringLength;
};

//
// C�������� ������� �������� �������� 
//
class ProcessDestroyEventInfo : public BaseEventInfo {
public:
    LONG64 ProcessId;
};

EXTERN_C_END