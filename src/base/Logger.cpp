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

#include "Logger.h"

#include "OSHelper.h"
#include "Exception.h"
#include "StandardLogSink.h"

#ifdef _WIN32
#include <Winsock2.h>
#include <time.h>
#include <Mmsystem.h>
#undef ERROR
#else
#include <sys/time.h>
#include <syslog.h>
#endif
#include <iostream>
#include <iomanip>
#include <boost/thread.hpp>
#include <boost/algorithm/string.hpp>

using namespace std;

namespace avg {

    const unsigned Logger::severity::CRITICAL = 50;
    const unsigned Logger::severity::ERROR = 40;
    const unsigned Logger::severity::WARNING = 30;
    const unsigned Logger::severity::INFO = 20;
    const unsigned Logger::severity::DEBUG = 10;

    const size_t Logger::category::NONE = 1;
    const size_t Logger::category::PROFILE = 2;
    const size_t Logger::category::PROFILE_VIDEO = 8;
    const size_t Logger::category::EVENTS = 16;
    const size_t Logger::category::CONFIG = 64;
    const size_t Logger::category::MEMORY = 512;
    const size_t Logger::category::APP = 1024;
    const size_t Logger::category::PLUGIN = 2048;
    const size_t Logger::category::PLAYER = 4096;
    const size_t Logger::category::SHADER = 8192;
    const size_t Logger::category::DEPRECATION = 16384;
    const size_t Logger::category::LAST_CATEGORY = DEPRECATION;

namespace {
    Logger* m_pLogger = 0;
    std::vector<LogSinkPtr> m_Sinks;
    boost::mutex logMutex;
    boost::mutex sinkMutex;
}

Logger * Logger::get()
{
    boost::mutex::scoped_lock lock(logMutex);
    if (!m_pLogger) {
        m_pLogger = new Logger;
    }
    return m_pLogger;
}

Logger::Logger()
{
    setupCategory();
    m_Severity = severity::INFO;
    string sEnvSeverity;
    bool bEnvSeveritySet = getEnv("AVG_LOG_SEVERITY", sEnvSeverity);
    if(bEnvSeveritySet){
        m_Severity = Logger::stringToSeverity(sEnvSeverity);
    }
    m_Flags = category::NONE | category::APP | category::DEPRECATION;
    string sEnvCategories;
    bool bEnvSet = getEnv("AVG_LOG_CATEGORIES", sEnvCategories);
    if (bEnvSet) {
        m_Flags = category::NONE;
        bool bDone = false;
        string sCategory;
        do {
            string::size_type pos = sEnvCategories.find(":");
            if (pos == string::npos) {
                sCategory = sEnvCategories;
                bDone = true;
            } else {
                sCategory = sEnvCategories.substr(0, pos);
                sEnvCategories = sEnvCategories.substr(pos+1);
            }
            size_t category = stringToCategory(sCategory);
            m_Flags |= category;
            } while (!bDone);
    }
    m_MaxCategoryNum = category::LAST_CATEGORY;

    string sDummy;
    bool bEnvUseStdErr = getEnv("AVG_LOG_OMIT_STDERR", sDummy);
    if (!bEnvUseStdErr) {
        addLogSink(LogSinkPtr(new StandardLogSink));
    }
}

Logger::~Logger()
{
}

size_t Logger::getCategories() const
{
    boost::mutex::scoped_lock lock(logMutex);
    return m_Flags;
}

void Logger::setCategories(size_t flags)
{
    boost::mutex::scoped_lock lock(logMutex);
    m_Flags = flags;
}

void Logger::setLogSeverity(unsigned severity){
        m_Severity = severity;
}

void Logger::pushCategories()
{
    boost::mutex::scoped_lock lock(logMutex);
    m_FlagStack.push_back(m_Flags);
}

void Logger::popCategories()
{
    boost::mutex::scoped_lock lock(logMutex);
    if (m_FlagStack.empty()) {
        throw Exception(AVG_ERR_OUT_OF_RANGE, "popCategories: Nothing to pop.");
    }
    m_Flags = m_FlagStack.back();
    m_FlagStack.pop_back();
}

void Logger::addLogSink(const LogSinkPtr& logSink)
{
    boost::mutex::scoped_lock lock(sinkMutex);
    m_Sinks.push_back(logSink);
}

void Logger::removeLogSink(const LogSinkPtr& logSink)
{
    boost::mutex::scoped_lock lock(sinkMutex);
    std::vector<LogSinkPtr>::iterator it;
    it = find(m_Sinks.begin(), m_Sinks.end(), logSink);
    if ( it != m_Sinks.end() ){
        m_Sinks.erase(it);
    }
}

void Logger::trace(const UTF8String& sMsg, size_t category, unsigned severity) const
{
    boost::mutex::scoped_lock lock(logMutex);
    struct tm* pTime;
    #ifdef _WIN32
    __int64 now;
    _time64(&now);
    pTime = _localtime64(&now);
    DWORD tms = timeGetTime();
    unsigned millis = unsigned(tms % 1000);
    #else
    struct timeval time;
    gettimeofday(&time, NULL);
    pTime = localtime(&time.tv_sec);
    unsigned millis = time.tv_usec/1000;
    #endif
    string sCategory = categoryToString(category);
    boost::mutex::scoped_lock lockHandler(sinkMutex);
    std::vector<LogSinkPtr>::iterator it;
    for(it=m_Sinks.begin(); it!=m_Sinks.end(); ++it){
        (*it)->logMessage(pTime, millis, sCategory, severity, sMsg);
    }
}

size_t Logger::registerCategory(const string& cat){
    std::map<const string, size_t >::iterator it;
    it = m_StringToCategory.find(cat);
    if(it != m_StringToCategory.end()){
        return it->second;
    }else{
        m_MaxCategoryNum *= 2;
        m_CategoryToString[m_MaxCategoryNum] = cat;
        m_StringToCategory[cat] = m_MaxCategoryNum;
        return m_MaxCategoryNum;
    }
}

void Logger::logDebug(const string& msg, const size_t category) const
{
    log(msg, category, Logger::severity::DEBUG);
}

void Logger::logInfo(const string& msg, const size_t category) const
{
    log(msg, category, Logger::severity::INFO);
}

void Logger::logWarning(const string& msg, const size_t category) const
{
    log(msg, category, Logger::severity::WARNING);
}

void Logger::logError(const string& msg, const size_t category) const
{
    log(msg, category, Logger::severity::ERROR);
}

void Logger::logCritical(const string& msg, const size_t category) const
{
    log(msg, category, Logger::severity::CRITICAL);
}

void Logger::log(const string& msg, const size_t category, unsigned severity) const
{
    if( shouldLog(category, severity)){
        trace(msg, category, severity);
    }
}

const char * Logger::categoryToString(size_t category) const
{
    std::map<const size_t, string>::const_iterator it;
    it = m_CategoryToString.find(category);
    if(it != m_CategoryToString.end()){
        return (it->second).c_str();
    }else{
        return "UNKNOWN";
    }
}

size_t Logger::stringToCategory(const string& sCategory) const
{
    std::map<const string , size_t >::const_iterator it;
    it = m_StringToCategory.find(sCategory);
    if(it != m_StringToCategory.end()){
        return it->second;
    }else{
        throw Exception (AVG_ERR_INVALID_ARGS, "Unknown logging category " + sCategory
                + " set using AVG_LOG_CATEGORIES.");
    }
}

void Logger::setupCategory(){
    m_CategoryToString[category::NONE] = "NONE";
    m_StringToCategory["NONE"] = category::NONE;

    m_CategoryToString[category::PROFILE] = "PROFILE",
    m_StringToCategory["PROFILE"] = category::PROFILE;

    m_CategoryToString[category::PROFILE_VIDEO] = "PROFILE_VIDEO";
    m_StringToCategory["PROFILE_VIDEO"] = category::PROFILE_VIDEO;

    m_CategoryToString[category::EVENTS] = "EVENTS";
    m_StringToCategory["EVENTS"] = category::EVENTS;

    m_CategoryToString[category::CONFIG] = "CONFIG";
    m_StringToCategory["CONFIG"] = category::CONFIG;

    m_CategoryToString[category::MEMORY] = "MEMORY";
    m_StringToCategory["MEMORY"] = category::MEMORY;

    m_CategoryToString[category::APP] = "APP";
    m_StringToCategory["APP"] = category::APP;

    m_CategoryToString[category::PLUGIN] = "PLUGIN";
    m_StringToCategory["PLUGIN"] = category::PLUGIN;

    m_CategoryToString[category::PLAYER] = "PLAYER";
    m_StringToCategory["PLAYER"] = category::PLAYER;

    m_CategoryToString[category::SHADER] = "SHADER";
    m_StringToCategory["SHADER"] = category::SHADER;

    m_CategoryToString[category::DEPRECATION] = "DEPRECATION";
    m_StringToCategory["DEPRECATION"] = category::DEPRECATION;
}

unsigned Logger::stringToSeverity(const string& sSeverity)
{
    string severity = boost::to_upper_copy(sSeverity);
    if (severity == "CRITICAL"){
        return Logger::severity::CRITICAL;
    }else if (severity == "ERROR"){
        return Logger::severity::ERROR;
    }else if (severity == "WARNING"){
        return Logger::severity::WARNING;
    }else if (severity == "INFO"){
        return Logger::severity::INFO;
    }else if (severity == "DEBUG"){
        return Logger::severity::DEBUG;
    }
    throw Exception(AVG_ERR_INVALID_ARGS, severity + " is an invalid log severity");
}

const char * Logger::severityToString(unsigned severity)
{
    if(severity == Logger::severity::CRITICAL){
        return "CRITICAL";
    }else if(severity == Logger::severity::ERROR){
        return "ERROR";
    }else if(severity == Logger::severity::WARNING){
        return "WARNING";
    }else if(severity == Logger::severity::INFO){
        return "INFO";
    }else if(severity == Logger::severity::DEBUG){
        return "DEBUG";
    }
    throw Exception(AVG_ERR_UNKNOWN, "Unkown log severity");
}


}
