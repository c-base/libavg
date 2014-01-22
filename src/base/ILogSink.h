//
//  libavg - Media Playback Engine.
//  Copyright (C) 2003-2014 Ulrich von Zadow
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

#ifndef _ILOGHANDLER_H_
#define _ILOGHANDLER_H_

#include "UTF8String.h"

#ifdef _WIN32
#include <time.h>
#else
#include <sys/time.h>
#endif

#include <boost/shared_ptr.hpp>

using namespace std;

namespace avg{

typedef unsigned severity_t;
typedef UTF8String category_t;

class AVG_API ILogSink
{
public:
    virtual void logMessage(const tm* pTime, unsigned millis, const category_t& category,
            severity_t severity, const UTF8String& sMsg) = 0;
    
};

typedef boost::shared_ptr<ILogSink> LogSinkPtr;

}

#endif
