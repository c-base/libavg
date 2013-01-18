//
//  libavg - Media Playback Engine. 
//  Copyright (C) 2003-2012 Ulrich von Zadow
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

#ifndef _PYTHONLOGHANDLER_H_
#define _PYTHONLOGHANDLER_H_

#include "../base/ILogHandler.h"
#include "../player/WrapPython.h"

namespace avg
{
class PythonLogHandler: public ILogHandler
{
public:
    PythonLogHandler(PyObject *pyLogger);
    virtual ~PythonLogHandler ();

    virtual void logMessage(tm* pTime, unsigned millis, int category,
            const UTF8String& sMsg);

private:

    PyObject *m_pyLogger;

};

}

#endif
