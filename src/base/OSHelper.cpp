//
//  libavg - Media Playback Engine. 
//  Copyright (C) 2003-2011 Ulrich von Zadow
//
//  This library is free software; you can redistribute it and/or
//  modify it under the terms of the GNU Lesser General Public
//  License as published by the Free Software Foundation; either
//  version 2 of the License, or (at your option) any later version.
//
//  This library is distributed in the hope that it will be useful,
//  but WITHOUT ANY WARRANTY; without even the implied warranty of
//  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
//  Lesser General Public License for more details.
//
//  You should have received a copy of the GNU Lesser General Public
//  License along with this library; if not, write to the Free Software
//  Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
//
//  Current versions can be found at www.libavg.de
//

#include "OSHelper.h"
#include "FileHelper.h"
#include "Logger.h"
#include "FileHelper.h"
#include "Exception.h"

#if defined(_WIN32)
#include <windows.h>
#include <psapi.h>
#undef ERROR
#undef WARNING
#elif defined(__APPLE__)
#include <mach-o/dyld.h>
#include <mach/mach.h>
#elif defined(__linux)
#include <fstream>
#include <unistd.h>
#endif

#include <stdlib.h>
#include <cstdlib>

using namespace std;

namespace avg {

#ifdef _WIN32
string getWinErrMsg(unsigned err) 
{
    LPVOID lpMsgBuf;
    FormatMessage(FORMAT_MESSAGE_ALLOCATE_BUFFER | FORMAT_MESSAGE_FROM_SYSTEM |
            FORMAT_MESSAGE_IGNORE_INSERTS,
            NULL, err, MAKELANGID(LANG_NEUTRAL, SUBLANG_DEFAULT), (LPTSTR) &lpMsgBuf,
            0, NULL );
    string sMsg((char*)lpMsgBuf);
    LocalFree(lpMsgBuf);
    return sMsg;
}
#endif

string getAvgLibPath()
{
#if defined(_WIN32)
    HMODULE hModule = GetModuleHandle("avg.pyd");
    char szFilename[1024];
    DWORD ok = GetModuleFileName(hModule, szFilename, sizeof(szFilename));
    if (ok == 0) {
        AVG_TRACE(Logger::ERROR, "getAvgLibPath(): " << getWinErrMsg(GetLastError()));
        exit(5);
    } 
    string sPath=getPath(szFilename);
    return sPath;
#elif defined(__APPLE__)
    // We need to iterate through all images attached to the current executable
    // and figure out which one is the one we are interested in.
    uint32_t numImages = _dyld_image_count();
    for (uint32_t i=0; i<numImages; i++) {
         const char * pszImageName = _dyld_get_image_name(i);
         string sFilePart=getFilenamePart(pszImageName);
         if (sFilePart == "avg.so" || sFilePart == "avg.0.so" 
                 || sFilePart == "avg.0.0.0.so") 
         {
             return getPath(pszImageName);
         }
    }
    char path[1024];
    uint32_t pathLen = sizeof(path);
    _NSGetExecutablePath(path, &pathLen);
    return getPath(path);
#else
    // For a linux solution, see http://www.autopackage.org/docs/binreloc/
    return "";
#endif
}

bool getEnv(const string & sName, string & sVal)
{
    const char * pszVal = getenv(sName.c_str());
    if (pszVal) {
        sVal = pszVal;
    }
    return (pszVal != 0);
}

void setEnv(const string & sName, const string & sVal)
{
#ifdef _WIN32
    SetEnvironmentVariable(sName.c_str(), sVal.c_str());
#else
    setenv(sName.c_str(), sVal.c_str(), true);
#endif
}

size_t getMemoryUsage()
{
#ifdef __APPLE__
    kern_return_t rc;
    mach_port_t task;
    rc = task_for_pid(mach_task_self(), getpid(), &task);
    AVG_ASSERT(rc == KERN_SUCCESS);
    struct task_basic_info taskInfo;
    mach_msg_type_number_t count = TASK_BASIC_INFO_COUNT;
    rc = task_info(task, TASK_BASIC_INFO, (task_info_t)&taskInfo, &count);
    AVG_ASSERT(rc == KERN_SUCCESS);
    return taskInfo.resident_size;
#else
#ifdef _WIN32
    DWORD pid = GetCurrentProcessId();
    HANDLE hProcess;
    PROCESS_MEMORY_COUNTERS pmc;
    hProcess = OpenProcess(PROCESS_QUERY_INFORMATION | PROCESS_VM_READ, FALSE, pid);
    BOOL bOk = GetProcessMemoryInfo(hProcess, &pmc, sizeof(pmc));
    CloseHandle(hProcess);
    return pmc.WorkingSetSize;
#else
    unsigned vmsize;
    unsigned rssize;
    // See 'man proc' for a documentation of this file's contents.
    std::ifstream f("/proc/self/statm");
    f >> vmsize >> rssize;
    return rssize*(size_t)(getpagesize());
#endif
#endif
}

std::string convertUTF8ToFilename(const std::string & sName)
{
#ifdef _WIN32
    // Conversion from utf-8 to something windows can use:
    // utf-8 long filename -> utf-16 long filename -> utf-16 short filename (8.3)
    // -> utf-8 short filename (= ASCII short filename).
    wchar_t wideString[2048];
    int err1 = MultiByteToWideChar(CP_UTF8, 0, sName.c_str(), sName.size()+1, 
            wideString, 2048);
    if (err1 == 0) {
        AVG_TRACE(Logger::WARNING, 
                "Error in unicode conversion (MultiByteToWideChar): " <<
                getWinErrMsg(GetLastError()));
        return sName;
    }
    wchar_t wideShortFName[2048];
    DWORD err2 = GetShortPathNameW(wideString, wideShortFName, 1024);
    if (err2 != 0) {
        char pShortName[1024];
        err1 = WideCharToMultiByte(CP_UTF8, 0, wideShortFName, -1, pShortName, 
                1024, 0, 0);
        if (err1 == 0) {
            AVG_TRACE(Logger::WARNING, 
                    "Error in unicode conversion (MultiByteToWideChar): " <<
                    getWinErrMsg(GetLastError()));
        }
        return pShortName;
    } else {
        return sName;
    }
#else
    return sName;
#endif
}

}
