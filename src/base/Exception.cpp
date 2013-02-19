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

#include "Exception.h"
#include "Backtrace.h"
#include "Logger.h"
#include "OSHelper.h"

#include <cstdlib>
#include <sstream>

#ifdef WIN32
#include <intrin.h>
#endif

using namespace std;

namespace avg {

Exception::Exception(int code, const string& sErr)
    : std::exception(),
      m_Code (code),
      m_sErr (sErr)
{
}

Exception::Exception(const Exception& ex)
    : std::exception(),
      m_Code (ex.getCode()),
      m_sErr (ex.getStr())
{
}

Exception::~Exception() throw()
{
}

int Exception::getCode() const
{
    return m_Code;
}

const string& Exception::getStr() const
{
    return m_sErr;
}

const char* Exception::what() const throw()
{
    return m_sErr.c_str();
}

void debugBreak()
{
#ifdef _WIN32
    __debugbreak();
#else
    __builtin_trap();
#endif
}

void avgAssert(bool b, const char * pszFile, int line, const char * pszReason)
{
    if (!b) {
        string sDummy;
        static bool bBreak = getEnv("AVG_BREAK_ON_ASSERT", sDummy);
        if (bBreak) {
            debugBreak();
        } else {
            stringstream ss;
            ss << "Assertion failed in " << pszFile << ": " << line << endl;
            if (pszReason) {
                ss << "Reason: " << pszReason << endl;
            }
            dumpBacktrace();
            throw(Exception(AVG_ERR_ASSERT_FAILED, ss.str()));
        }
    }
}

}
