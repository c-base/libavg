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

#include "MouseEvent.h"

#include "../base/Logger.h"
#include "../base/Exception.h"

#include <iostream>
#include <sstream>

using namespace std;

namespace avg {

MouseEvent::MouseEvent(Event::Type eventType,
        bool leftButtonState, bool middleButtonState, bool rightButtonState,
        const IntPoint& pos, int button, const glm::vec2& speed, int when)
    : CursorEvent(MOUSECURSORID, eventType, pos, MOUSE, when)
{
    m_LeftButtonState = leftButtonState;
    m_MiddleButtonState = middleButtonState;
    m_RightButtonState = rightButtonState;
    m_Button = button;
    setSpeed(speed);
}

MouseEvent::~MouseEvent()
{
}

int MouseEvent::getButton() const
{
    return m_Button;
}

bool MouseEvent::getLeftButtonState() const
{
    return m_LeftButtonState;
}

bool MouseEvent::getMiddleButtonState() const
{
    return m_MiddleButtonState;
}

bool MouseEvent::getRightButtonState() const
{
    return m_RightButtonState;
}

bool MouseEvent::isAnyButtonPressed() const
{
    return m_LeftButtonState || m_MiddleButtonState || m_RightButtonState;
}

void MouseEvent::trace()
{
    CursorEvent::trace();
    AVG_TRACE(Logger::category::EVENTS, Logger::severity::DEBUG, "pos: " << getPos() 
            << ", button: " << m_Button);
}

CursorEventPtr MouseEvent::cloneAs(Type EventType) const
{
    MouseEventPtr pClone(new MouseEvent(*this));
    pClone->m_Type = EventType;
    return pClone;
}

}
