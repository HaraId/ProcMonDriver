#pragma once

//DEFINE_GUID (GUID_DEVINTERFACE_ProcMonDriver,
//    0x8c148369,0xd3f7,0x47b2,0x99,0xe6,0x37,0x2d,0x3e,0x7d,0x98,0xc5);
// {8c148369-d3f7-47b2-99e6-372d3e7d98c5}

#define IOCTL_CONTROL_MESSAGE CTL_CODE  (FILE_DEVICE_UNKNOWN, 0x801, METHOD_BUFFERED, FILE_ANY_ACCESS)


EXTERN_C_START

enum class EventType : char {
    Undefined,
    ProcessCreate,
    ProcessDestroy,
    ThreadCreate,
    ThreadDestroy,
    LoadImage
};

class BaseInfo {
public:
    size_t Size;
    EventType Type;
};

class ProcessCreateInfo : public BaseInfo {
public:
    LONG64 ProcessId;
    LONG64 ParentProcessId;
    LONG64 ParentThreadId;

    LONG64 ImageFileNameOffset;
    LONG64 ImageFileNameLength;

    LONG64 CommandLineStringOffset;
    LONG64 CommandLineStringLength;
};

class ProcessDestroyInfo : public BaseInfo {
public:
    LONG64 ProcessId;
};

EXTERN_C_END