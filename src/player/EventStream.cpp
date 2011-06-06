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

#include "EventStream.h"
#include "TouchEvent.h"

#include "../base/Logger.h"
#include "../base/ObjectCounter.h"
#include "../base/Exception.h"

#include <math.h>

using namespace std;

namespace avg {
    
int EventStream::s_LastLabel = 0;

EventStream::EventStream(BlobPtr pFirstBlob, long long time)
    : m_Time(time)
{
    ObjectCounter::get()->incRef(&typeid(*this));
    m_ID = ++s_LastLabel;
    m_pBlob = pFirstBlob;
    m_Pos = m_pBlob->getCenter();
    m_OldPos = m_Pos;
    m_State = DOWN_PENDING;
    m_Stale = false;
    m_OldTime = 0;
}

EventStream::~EventStream()
{
    ObjectCounter::get()->decRef(&typeid(*this));
}

void EventStream::blobChanged(BlobPtr pNewBlob, long long time, bool bKeepEvent)
{
    AVG_ASSERT(m_pBlob);
    AVG_ASSERT(pNewBlob);
    DPoint c = pNewBlob->getCenter();
    bool bPosChanged;
    if (bKeepEvent) {
        bPosChanged = (calcDist(c, m_Pos) > 1);
    } else {
        bPosChanged = true;
    }
    switch (m_State) {
        case DOWN_PENDING:
            //finger touch has not been polled yet. update position
            break;
        case VANISHED:
            m_State = MOTION_PENDING;
            break;
        case DOWN_DELIVERED:
            //fingerdown delivered, change to motion states
            if (bPosChanged)
                m_State = MOTION_PENDING;
            else
                m_State = MOTION_DELIVERED;
            break;
        case MOTION_PENDING:
            break;
        case MOTION_DELIVERED:
            if (bPosChanged) {
                m_State = MOTION_PENDING;
            }
            break;
        default:
            //pass
            break;
    }
    if (bPosChanged) {
        m_OldTime = m_Time;
        m_Time = time;
        m_OldPos = m_Pos;
        m_Pos = c;
    }
    m_pBlob = pNewBlob;
    m_Stale = false;
};
    
void EventStream::blobGone()
{
    switch (m_State) {
        case DOWN_PENDING:
            m_State = UP_DELIVERED;
            break;
        case UP_PENDING:
        case UP_DELIVERED:
            break;
        default:
            m_State = UP_PENDING;
            break;
    }
}

EventPtr EventStream::pollevent(DeDistortPtr pDeDistort, const DRect& displayROI, 
        CursorEvent::Source source)
{
    AVG_ASSERT(m_pBlob);
    DPoint BlobOffset = pDeDistort->getActiveBlobArea(displayROI).tl;
    DPoint pt = m_pBlob->getCenter()+BlobOffset;
    DPoint screenpos = pDeDistort->transformBlobToScreen(pt);
    IntPoint pos(int(screenpos.x+0.5), int(screenpos.y+0.5)); 
    DPoint oldPos = pDeDistort->transformBlobToScreen(m_OldPos + BlobOffset);
    DPoint newPos = pDeDistort->transformBlobToScreen(m_Pos + BlobOffset);
    DPoint speed = getSpeed(oldPos, newPos);
    switch (m_State) {
        case DOWN_PENDING:
            m_State = DOWN_DELIVERED;
            return EventPtr(new TouchEvent(m_ID, Event::CURSORDOWN,
                    m_pBlob, pos, source, speed));
        case MOTION_PENDING:
            m_State = MOTION_DELIVERED;
            return EventPtr(new TouchEvent(m_ID, Event::CURSORMOTION,
                    m_pBlob, pos, source, speed));
        case UP_PENDING:
            m_State = UP_DELIVERED;
            return EventPtr(new TouchEvent(m_ID, Event::CURSORUP,
                    m_pBlob, pos, source, speed));
        case DOWN_DELIVERED:
        case MOTION_DELIVERED:
            return EventPtr(new TouchEvent(m_ID, Event::CURSORMOTION,
                    m_pBlob, pos, source, speed));
        case UP_DELIVERED:
        default:
            //return no event
            return EventPtr();
    }
};

bool EventStream::isGone()
{
    return (m_State == UP_DELIVERED);
}

void EventStream::setStale() {
    m_Stale = true;
}

bool EventStream::isStale() {
    return m_Stale;
}

string EventStream::stateToString(StreamState State) 
{
    switch(State) {
        case DOWN_PENDING:
            return "DOWN_PENDING";
        case DOWN_DELIVERED:
            return "DOWN_DELIVERED";
        case MOTION_PENDING:
            return "MOTION_PENDING";
        case MOTION_DELIVERED:
            return "MOTION_DELIVERED";
        case VANISHED:
            return "VANISHED";
        case UP_PENDING:
            return "UP_PENDING";
        case UP_DELIVERED:
            return "UP_DELIVERED";
        default:
            return "Broken state";
    }
}

void EventStream::dump() 
{
    cerr << "  " << m_ID << ": " << stateToString(m_State) << ", stale: " 
            << m_Stale << endl;
}

DPoint EventStream::getSpeed(const DPoint& oldPos, const DPoint& newPos)
{
    if (m_OldTime==0 || m_Time == m_OldTime) {
        return DPoint(0,0);
    } else {
        double timeDiff = double(m_OldTime-m_Time);
        return (oldPos-newPos)/timeDiff;
    }
}

}
