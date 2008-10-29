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

#ifndef _EventDispatcher_h_
#define _EventDispatcher_h_

#include "../api.h"
#include "IEventSink.h"
#include "IEventSource.h"
#include "MouseEvent.h"

#include <vector>
#include <queue>
#include <string>

namespace avg {

class Event;

class AVG_API EventDispatcher {
    public:
        EventDispatcher();
        virtual ~EventDispatcher();
        void dispatch();
        
        void addSource(IEventSource * pSource);
        void addSink(IEventSink * pSink);

        void sendEvent(EventPtr pEvent);

    private:
        void handleEvent(EventPtr pEvent);

        std::vector<IEventSource*> m_EventSources;
        std::vector<IEventSink*> m_EventSinks;
};
typedef boost::shared_ptr<EventDispatcher> EventDispatcherPtr;

}

#endif

