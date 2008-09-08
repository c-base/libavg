//
//  libavg - Media Playback Engine. 
//  Copyright (C) 2003-2008 Ulrich von Zadow
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

#include "Logger.h"

#ifdef _WIN32
#define WIN32_LEAN_AND_MEAN  /* somewhat limit Win32 pollution */
#include <Winsock2.h>
#undef ERROR
#undef WARNING
#include <time.h>
#else
#include <sys/time.h>
#include <syslog.h>
#endif
#include <fstream>
#include <iomanip>
#include <boost/thread.hpp>
using namespace std;

namespace avg {

const long Logger::NONE=0;
const long Logger::BLTS=1;
const long Logger::PROFILE=2;
const long Logger::PROFILE_LATEFRAMES=4;
const long Logger::EVENTS=8;
const long Logger::EVENTS2=16;
const long Logger::CONFIG=32;  
const long Logger::WARNING=64;
const long Logger::ERROR=128;  
const long Logger::MEMORY=256;
const long Logger::APP=512;
const long Logger::LOGGER=1024;
const long Logger::WATCHDOG=2048;

Logger* Logger::m_pLogger = 0;
boost::mutex log_Mutex;

Logger * Logger::get()
{

    if (!m_pLogger) {
    {
        boost::mutex::scoped_lock Lock(log_Mutex);
        m_pLogger = new Logger;
    }
        m_pLogger->trace(LOGGER, "Logging started ");
    }
    return m_pLogger;
}

Logger::Logger()
{
    m_Flags = ERROR | WARNING | APP | LOGGER;
    m_DestType = CONSOLE;
    m_pDest = &cerr;
}

Logger::~Logger()
{
    closeDest();
}

void Logger::setConsoleDest()
{
    boost::mutex::scoped_lock Lock(log_Mutex);
    closeDest();
    m_DestType = CONSOLE;
    m_pDest = &cerr;
    AVG_TRACE(LOGGER, "Logging started ");
}

void Logger::setFileDest(const std::string& sFName)
{
    {
        boost::mutex::scoped_lock Lock(log_Mutex);
        closeDest();
        m_DestType = FILE;
        m_pDest = new ofstream(sFName.c_str(), ios::out | ios::app);
    }
    if (!*m_pDest) {
        m_pDest = &cerr;
        m_DestType = CONSOLE;
        AVG_TRACE(LOGGER, "Could not open " << sFName 
                << " as log destination.");
    } else {
        AVG_TRACE(LOGGER, "Logging started ");
    }
}

void Logger::setSyslogDest(int facility, int logopt)
{
#ifdef _WIN32
    AVG_TRACE(ERROR, 
            "Logging to syslog is not supported under windows. Ignoring request.");
#else
    boost::mutex::scoped_lock Lock(log_Mutex);
    closeDest();
    m_DestType = SYSLOG;
    openlog("libavg", logopt, facility);
#endif
}
    
void Logger::setCategories(int flags)
{
    boost::mutex::scoped_lock Lock(log_Mutex);
    m_Flags = flags | ERROR | APP;
}

void Logger::trace(int category, const std::string& msg)
{
    boost::mutex::scoped_lock Lock(log_Mutex);
    if (category & m_Flags) {
        if (m_DestType == CONSOLE || m_DestType == FILE) {
            struct tm* pTime;
#ifdef _WIN32
            __int64 now;
            _time64(&now);
            pTime = _localtime64(&now);
            unsigned millis = unsigned((now / 1000) % 1000);
#else
            struct timeval time;
            gettimeofday(&time, NULL);
            pTime = localtime(&time.tv_sec);
            unsigned millis = time.tv_usec/1000;
#endif
            char timeString[256];
            strftime(timeString, sizeof(timeString), "%y-%m-%d %H:%M:%S", pTime);
            (*m_pDest) << "[" << timeString << "." << 
                setw(3) << setfill('0') << millis << setw(0) << "] ";
            (*m_pDest) << categoryToString(category) << ": ";
            (*m_pDest) << msg << endl;
            m_pDest->flush();
        } else {
#ifndef _WIN32
            int prio;
            switch(category) {
                case EVENTS:
                case EVENTS2:
                case BLTS:
                    prio = LOG_INFO;
                    break;
                case PROFILE:
                case PROFILE_LATEFRAMES:
                case CONFIG:
                case APP:
                case LOGGER:
                case MEMORY:
                    prio = LOG_NOTICE;
                    break;
                case WARNING:
                    prio = LOG_WARNING;
                    break;
                case ERROR:
                    prio = LOG_ERR;
                    break;
                default:
                    prio = LOG_ERR;
            }
            syslog(prio, "%s: %s", categoryToString(category),
                    msg.c_str());
#endif 
        }
    }
}

const char * Logger::categoryToString(int category)
{
    switch(category) {
        case BLTS:
            return "BLIT";
        case PROFILE:
        case PROFILE_LATEFRAMES:
            return "PROFILE";
        case EVENTS:
        case EVENTS2:
            return "EVENTS";
        case CONFIG:
            return "CONFIG";
        case WARNING:
            return "WARNING";
        case ERROR:
            return "ERROR";
        case MEMORY:
            return "MEMORY";
        case APP:
            return "APP";
        case LOGGER:
            return "LOGGER";
        case WATCHDOG:
            return "WATCHDOG";
        default:
            return "UNKNOWN";
    }
}

void Logger::closeDest()
{
    switch (m_DestType) {
        case CONSOLE:
            break;
        case FILE:
            delete m_pDest;
            m_pDest = 0;
            break;
        case SYSLOG:
#ifndef _WIN32
            closelog();
#endif
            break;
    }
}

}
