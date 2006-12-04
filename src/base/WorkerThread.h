//
//  libavg - Media Playback Engine. 
//  Copyright (C) 2003-2006 Ulrich von Zadow
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

#ifndef _WorkerThread_H_
#define _WorkerThread_H_

#include "Command.h"
#include "Queue.h"

namespace avg {


template<class DERIVED_THREAD>
class WorkerThread {
public:
    typedef Queue<Command<DERIVED_THREAD> > CmdQueue;

    WorkerThread(CmdQueue& CmdQ);
    virtual ~WorkerThread() {};
    void operator()();

    void stop();

private:
    virtual bool init() = 0;
    virtual bool work() = 0;
    virtual void deinit() = 0;

    void processCommands();

    bool m_bShouldStop;
    CmdQueue& m_CmdQ;
};

template<class DERIVED_THREAD>
WorkerThread<DERIVED_THREAD>::WorkerThread(CmdQueue& CmdQ)
    : m_bShouldStop(false),
      m_CmdQ(CmdQ)
{
}

template<class DERIVED_THREAD>
void WorkerThread<DERIVED_THREAD>::operator()()
{
    bool bOK;
    bOK = init();
    if (!bOK) {
        return;
    }
    while (!m_bShouldStop) {
        bOK = work();
        if (!bOK) {
            m_bShouldStop = true;
        } else {
            processCommands();
        }
    }
    deinit();
}

template<class DERIVED_THREAD>
void WorkerThread<DERIVED_THREAD>::processCommands()
{
    try {
        // This loop always ends in an exception when the Queue is empty.
        while (true) {
            Command<DERIVED_THREAD> Cmd = m_CmdQ.pop(false);
            Cmd.execute(dynamic_cast<DERIVED_THREAD*>(this));
        }
    } catch (Exception& ex) {
    }
   
}

template<class DERIVED_THREAD>
void WorkerThread<DERIVED_THREAD>::stop() {
    m_bShouldStop = true;
}

}

#endif
