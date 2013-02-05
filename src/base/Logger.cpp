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
#include "StandardLoggingHandler.h"

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

    const unsigned Logger::level::CRITICAL = 50;
    const unsigned Logger::level::ERROR = 40;
    const unsigned Logger::level::WARNING = 30;
    const unsigned Logger::level::INFO = 20;
    const unsigned Logger::level::DEBUG = 10;

    const size_t Logger::category::NONE = 1;
    const size_t Logger::category::PROFILE = 2;
    const size_t Logger::category::PROFILE_VIDEO = 8;
    const size_t Logger::category::EVENTS = 16;
    const size_t Logger::category::EVENTS2 = 32;
    const size_t Logger::category::CONFIG = 64;
    const size_t Logger::category::MEMORY = 512;
    const size_t Logger::category::APP = 1024;
    const size_t Logger::category::PLUGIN = 2048;
    const size_t Logger::category::PLAYER = 4096;
    const size_t Logger::category::SHADER = 8192;
    const size_t Logger::category::DEPRECATION = 16384;

namespace {
    Logger* m_pLogger = 0;
    std::vector<LogHandlerPtr> m_Handlers;
    boost::mutex logMutex;
    boost::mutex handlerMutex;
}

unsigned Logger::stringToLevel(const string& sLevel)
{
    string level = boost::to_upper_copy(sLevel);
    if (level == "CRITICAL"){
        return Logger::level::CRITICAL;
    }else if (level == "ERROR"){
        return Logger::level::ERROR;
    }else if (level == "WARNING"){
        return Logger::level::WARNING;
    }else if (level == "INFO"){
        return Logger::level::INFO;
    }else if (level == "DEBUG"){
        return Logger::level::DEBUG;
    }
    throw Exception(AVG_ERR_INVALID_ARGS, level + " is an invalid log level");
}

const char * Logger::levelToString(unsigned level)
{
    if(level == Logger::level::CRITICAL){
        return "CRITICAL";
    }else if(level == Logger::level::ERROR){
        return "ERROR";
    }else if(level == Logger::level::WARNING){
        return "WARNING";
    }else if(level == Logger::level::INFO){
        return "INFO";
    }else if(level == Logger::level::DEBUG){
        return "DEBUG";
    }
    throw Exception(AVG_ERR_UNKNOWN, "Unkown log level");
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
    m_Level = level::INFO;
    string sEnvLevel;
    bool bEnvLevelSet = getEnv("AVG_LOG_LEVEL", sEnvLevel);
    if(bEnvLevelSet){
        m_Level = Logger::stringToLevel(sEnvLevel);
    }
    m_Flags = category::NONE | category::APP | category::DEPRECATION;
    string sEnvCategories;
    bool bEnvSet = getEnv("AVG_LOG_CATEGORIES", sEnvCategories);
    if (bEnvSet) {
        m_Flags = category::NONE | category::DEPRECATION;
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
    m_MaxCategoryNum = category::DEPRECATION;

    string sDummy;
    bool bEnvUseStdErr = getEnv("AVG_LOG_OMIT_STDERR", sDummy);
    if (!bEnvUseStdErr) {
        addLogHandler(LogHandlerPtr(new StandardLoggingHandler));
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
    m_Flags = flags | category::DEPRECATION;
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

void Logger::addLogHandler(const LogHandlerPtr& logHandler)
{
    boost::mutex::scoped_lock lock(handlerMutex);
    m_Handlers.push_back(logHandler);
}

void Logger::pytrace(size_t category, const UTF8String& sMsg, unsigned level) const
{
    //TODO: Use avg_deprecation
    AVG_TRACE(Logger::category::DEPRECATION, Logger::level::WARNING,
            "logger.trace will be removed in future versions. Please use logger.log or any of the convenience functions");
    trace(sMsg, category, level);
}

void Logger::trace(const UTF8String& sMsg, size_t category, unsigned level) const
{
    boost::mutex::scoped_lock lock(logMutex);
    if ((m_Level <= level && category & m_Flags) || Logger::level::ERROR <= level) {
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
        boost::mutex::scoped_lock lock(handlerMutex);
        std::vector<LogHandlerPtr>::iterator it;
        for(it=m_Handlers.begin(); it!=m_Handlers.end(); ++it){
            (*it)->logMessage(pTime, millis, sCategory, level, sMsg);
        }
    }
}

void Logger::logDebug(const string& msg, const size_t category) const
{
    trace(msg, category, Logger::level::DEBUG);
}

void Logger::logInfo(const string& msg, const size_t category) const
{
    trace(msg, category, Logger::level::INFO);
}

void Logger::logWarning(const string& msg, const size_t category) const
{
    trace(msg, category, Logger::level::WARNING);
}

void Logger::logError(const string& msg, const size_t category) const
{
    trace(msg, category, Logger::level::ERROR);
}

void Logger::logCritical(const string& msg, const size_t category) const
{
    trace(msg, category, Logger::level::CRITICAL);
}

void Logger::log(const string& msg, const size_t category, unsigned level) const
{
    trace(msg, category, level);
}

const char * Logger::categoryToString(size_t category) const
{
    std::map< size_t, string >::const_iterator it;
    it = m_CategoryToString.find(category);
    if(it != m_CategoryToString.end()){
        return (it->second).c_str();
    }else{
        return "UNKNOWN";
    }
}

size_t Logger::stringToCategory(const string& sCategory) const
{
    std::map< string , size_t >::const_iterator it;
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

    m_CategoryToString[category::EVENTS2] = "EVENTS2";
    m_StringToCategory["EVENTS2"] = category::EVENTS2;

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

size_t Logger::registerCategory(const string& cat){
    std::map< string, size_t >::iterator it;
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

}
