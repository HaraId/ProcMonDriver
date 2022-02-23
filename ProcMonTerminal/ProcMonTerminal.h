#pragma once

#include <iostream>
#include <fstream>
#include <Windows.h>
#include <conio.h>
#include <string>
#include <list>
#include <map>
#include "../ProcMonDriver/Public.h"

using namespace std;


#define DRIVER_NAME "ProcMonDriver"
#define DRIVER_SYM_NAME "\\\\.\\ProcMon"
#define DRIVER_FILE_NAME "ProcMonDriver.sys"

#define DUMP_FILE_NAME "process-dump.txt"



class ProcessInformationItem {

public:
    ProcessInformationItem() : ProcessID(0), ParentProcessId(0), ParentThreadId(0), isTerminated(false), noCreationInfo(false) {}

    LONG64 ProcessID;
    LONG64 ParentProcessId;
    LONG64 ParentThreadId;

    bool isTerminated;
    bool noCreationInfo;

    std::string ImageName;
    std::string CommandLine;

    friend std::ostream& operator <<(std::ostream& os, const ProcessInformationItem& pii)
    {
        os << "ProcessID: " << pii.ProcessID << " " << (pii.isTerminated ? "[ was terminated ]:" : "[ in process... ]:") << endl;

        if (!pii.noCreationInfo)
        {
            os << "Image Name: " << pii.ImageName << ";" << endl;
            os << "Parent process ID: " << pii.ParentProcessId << ";" << endl;
            os << "Parent thread ID: " << pii.ParentThreadId << ";" << endl;
            os << "Command line: " << pii.CommandLine << "." << endl;
        }
        else
        {
            os << "no more info." << endl;
        }

        return os;
    }
};


struct ProcessInformations {
    std::map<LONG64, ProcessInformationItem> workingProcesses;
    std::list<ProcessInformationItem> terminatedProcesses;
};


//
// Driver load helpfull functions
//
HANDLE attemptOpenKernelDriver(const std::string& DriverSymName, DWORD* Status);
bool loadNonPnpDriver(const std::string& DriverFileName, const std::string& DriverPath, SC_HANDLE* SCManager, SC_HANDLE* ServiceHandle);
bool unloadNonPnpDriver(const SC_HANDLE SCManager, const SC_HANDLE ServiceHandle);


std::string convertUnicodeToAsci(const char* Unicode, const size_t Size);


char waitToPressAnyKey();
char wiatToPress—ertainKey(const std::string& Keys);


std::string GetAbsFilePath(const std::string& FileName);


void parseReceiveData(const unsigned char* Data, const size_t Size, ProcessInformations& AllProcessInfo);
void dumpAllInfoInFile(const ProcessInformations& AllProcessInfo);