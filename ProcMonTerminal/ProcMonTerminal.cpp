#include "ProcMonTerminal.h"



int main()
{
    DWORD status;

    const size_t transportBufferSize = 20000;
    DWORD numberOfBytesRead = 0;
    unsigned char *transportBuffer = new unsigned char[transportBufferSize];

    char controlInputCode;

    HANDLE hDriver = INVALID_HANDLE_VALUE;
    bool driverWasLoaded = false;
    SC_HANDLE scManager = NULL;
    SC_HANDLE serviceHandle = NULL;

    size_t readInterval = 200; // mcs
    bool dumpFile = true;      // 

    ProcessInformations allProcessInfo;


    std::cout << "           --- ProcMonTerminal ---               " << endl;
    std::cout << " To start monitoring press key 's';              " << endl;
    std::cout << " Press 'm' to get access for options;            " << endl;
    std::cout << " Press 'q' to stop monitoring, dump log and exit." << endl << endl;

    std::cout << ". ReadInterval = " << readInterval << endl;
    std::cout << ". DumpFile = " << (dumpFile ? "Yes" : "No") << endl << endl;
    
    while ((controlInputCode = wiatToPressСertainKey("smq")) != 's')
    {
        if (controlInputCode == 'q')
            goto __exit;

        if (controlInputCode == 'm')
        {
            size_t newReadInterval;
            std::cout << "Current read interval: " << readInterval << " mcs" << endl;
            std::cout << "New read interval (mcs < 5000): ";
            cin >> newReadInterval;

            std::cout << "Dump File (y/n): ";
            dumpFile = (wiatToPressСertainKey("yn") == 'y');
            std::cout << (dumpFile ? "Yes" : "No") << endl;

            if (newReadInterval < 5000)
                readInterval = newReadInterval;
        }
    }

    hDriver = attemptOpenKernelDriver(DRIVER_SYM_NAME, &status);
    // Драйвер успешно открыт
    if (hDriver != INVALID_HANDLE_VALUE) {
        std::cout << "Driver already loaded. Open was successful." << endl;
    }
    // Драйвер не обнаружен, осуществлуем попутку загрузки его в память
    else if (status == ERROR_FILE_NOT_FOUND) 
    {
        std::cout << "Driver not loaded, trying to load it...";

        const std::string driverAbsPath = GetAbsFilePath(DRIVER_FILE_NAME);
        if (driverAbsPath.empty()) {
            std::cout << "Driver file not found (" << DRIVER_FILE_NAME << ")." << endl;
            goto __error_exit;
        }

        if (!loadNonPnpDriver(DRIVER_FILE_NAME, driverAbsPath, &scManager, &serviceHandle)){
            std::cout << "Can't load driver, may be access denied." << endl;
            goto __error_exit;
        }

        driverWasLoaded = true;

        hDriver = attemptOpenKernelDriver(DRIVER_SYM_NAME, &status);

        if (hDriver == INVALID_HANDLE_VALUE) {
            std::cout << "Unsuccessful attempt to download the driver" << endl;
            goto __error_exit;
        }
        
        std::cout << "Driver loaded successful." << endl;
    }
    // Неизвесная ошибка во время открытия драйвера...
    else {
        std::cout << "Application will terminated, because CreateFile return " << status << "." << endl;
        goto __error_exit;
    }

    
    if (!DeviceIoControl(hDriver, IOCTL_CONTROL_MESSAGE_START, NULL, NULL, NULL, NULL, NULL, NULL)) {
        std::cout << "Error: DeviceIoControl return: " << GetLastError() << endl;
        goto __error_exit;
    }


    while (true)
    {
        Sleep(readInterval);

        if (_kbhit() && _getch() == 'q')
            goto __exit;

        if (!ReadFile(hDriver, transportBuffer, transportBufferSize, &numberOfBytesRead, NULL)) {
            std::cout << "Error ReadFile return: " << GetLastError() << endl;
            goto __error_exit;
        }

        std::cout << ".NumberOfBytesRead: " << numberOfBytesRead << endl;

        if (!numberOfBytesRead)
            continue;

        parseReceiveData(transportBuffer, numberOfBytesRead, allProcessInfo);
    }


__error_exit:

__exit: 
    if (hDriver != INVALID_HANDLE_VALUE)
        CloseHandle(hDriver);

    if (driverWasLoaded)
        unloadNonPnpDriver(scManager, serviceHandle);

    if (dumpFile)
        dumpAllInfoInFile(allProcessInfo);

    if (transportBuffer != nullptr)
        delete[] transportBuffer;

    std::cout << "Pouse... Press any key." << endl;
    waitToPressAnyKey();

    return 0;
}


HANDLE attemptOpenKernelDriver(const std::string& DriverName, DWORD* Status)
{
    HANDLE hHandl = CreateFileA(DriverName.c_str(),
        GENERIC_READ | GENERIC_WRITE,
        FILE_SHARE_READ | FILE_SHARE_WRITE,
        NULL, OPEN_EXISTING,
        FILE_ATTRIBUTE_NORMAL, NULL
    );

    if (hHandl == INVALID_HANDLE_VALUE)
        *Status = GetLastError();
    else
        *Status = ERROR_SUCCESS;

    return hHandl;
}

bool loadNonPnpDriver(const std::string& DriverFileName, const std::string& DriverPath, SC_HANDLE* SCManager, SC_HANDLE* ServiceHandle)
{
    SC_HANDLE sch = OpenSCManager(NULL, NULL, SC_MANAGER_ALL_ACCESS);
    if (sch == NULL) {
        // It may be ERROR_ACCESS_DENIED... <- GetLastError();
        std::cout << "Error (loadNonPnpDriver:OpenSCManager) status: " << GetLastError() << endl;
        return false;
    }

    SC_HANDLE schService = CreateServiceA(sch, DriverFileName.c_str(),
        DriverFileName.c_str(), SERVICE_ALL_ACCESS,
        SERVICE_KERNEL_DRIVER, SERVICE_DEMAND_START, SERVICE_ERROR_NORMAL,
        DriverPath.c_str(), NULL, NULL, NULL, NULL, NULL);
    if (schService == NULL) {
        CloseServiceHandle(sch);
        std::cout << "Error (loadNonPnpDriver:CreateServiceA) status: " << GetLastError() << endl;
        return false;
    }

    if (!StartService(schService, NULL, NULL)) {
        DeleteService(schService) && CloseServiceHandle(schService) && CloseServiceHandle(sch);
        std::cout << "Error (loadNonPnpDriver:StartService) status: " << GetLastError() << endl;
        return false;
    }

    *SCManager = sch;
    *ServiceHandle = schService;

    return true;
}


bool unloadNonPnpDriver(const SC_HANDLE SCManager, const SC_HANDLE ServiceHandle)
{
    SERVICE_STATUS status;

    return ControlService(ServiceHandle, SERVICE_CONTROL_STOP, &status) && DeleteService(ServiceHandle)
        && CloseServiceHandle(ServiceHandle) && CloseServiceHandle(SCManager);
}


std::string convertUnicodeToAsci(const char* Unicode, const size_t Size)
{
    std::string res;
    res.reserve(Size / 2);

    for (int i = 0; i < Size && isprint(Unicode[i]); i += 2)
        res += Unicode[i];
    
    return res;
}


char waitToPressAnyKey(){
    return _getch();
}


char wiatToPressСertainKey(const std::string& Keys)
{
    while (true)
    {
        char key = waitToPressAnyKey();
        if (Keys.find(key) != std::string::npos)
            return key;
    }
}


std::string GetAbsFilePath(const std::string& FileName)
{
    char fullPath[MAX_PATH];

    if (0 == GetFullPathNameA(FileName.c_str(), MAX_PATH, fullPath, NULL))
        return "";

    return std::string(fullPath);
}


void parseReceiveData(const unsigned char* Data, const size_t Size, ProcessInformations& AllProcessInfo)
{
    for (int i = 0; i < Size; ) {
        BaseEventInfo* itemHeader = (BaseEventInfo*)(&Data[i]);

        if (itemHeader->Size > Size - i)
            return;

        //
        // Событие создания нового процесса
        //
        if (itemHeader->Type == EventType::ProcessCreate)
        {
            ProcessCreateEventInfo* item = (ProcessCreateEventInfo*)itemHeader;

            // Добавляем процесс в таблицу работающих процессов
            auto& elem = AllProcessInfo.workingProcesses[item->ProcessId];

            elem.ProcessID = item->ProcessId;
            elem.ParentProcessId = item->ParentProcessId;
            elem.ParentThreadId = item->ParentThreadId;

            elem.ImageName = convertUnicodeToAsci(PCHAR(item) + item->ImageFileNameOffset, item->ImageFileNameLength);
            elem.CommandLine = convertUnicodeToAsci(PCHAR(item) + item->CommandLineStringOffset, item->CommandLineStringLength);

            std::cout << "[CreateProcess] " << elem << endl;

            i += item->Size;
        }
        
        //
        // Событие удаления процесса
        //
        else if (itemHeader->Type == EventType::ProcessDestroy)
        {
            ProcessDestroyEventInfo* item = (ProcessDestroyEventInfo*)itemHeader;

            auto workingElement = AllProcessInfo.workingProcesses.find(item->ProcessId);
            
            if (workingElement != AllProcessInfo.workingProcesses.end())
            {
                workingElement->second.isTerminated = true;

                AllProcessInfo.terminatedProcesses.push_back(workingElement->second);
                AllProcessInfo.workingProcesses.erase(workingElement);
            }
            else
            {
                ProcessInformationItem pii;
                pii.ProcessID = item->ProcessId;
                pii.noCreationInfo = true;
                pii.isTerminated = true;
                AllProcessInfo.terminatedProcesses.push_back(pii);
            }

            std::cout << "[DestroyProcess] ProcessID: " << item->ProcessId << endl;

            i += item->Size;
            std::cout << endl;
        }
    }
}


void dumpAllInfoInFile(const ProcessInformations& AllProcessInfo)
{
    std::ofstream fout(DUMP_FILE_NAME);

    fout << "Processing: " << AllProcessInfo.workingProcesses.size() << endl
        << "Terminated: " << AllProcessInfo.terminatedProcesses.size() << endl << endl;

    for (auto elem : AllProcessInfo.workingProcesses)
        fout << elem.second << endl;

    for (auto elem : AllProcessInfo.terminatedProcesses)
        fout << elem << endl;
}
