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

#ifndef _MouseEvent_h_
#define _MouseEvent_h_

#include "Event.h"
#include "Node.h"

namespace avg {

class MouseEvent : public Event {
    public:
        MouseEvent(Event::Type eventType,
                bool leftButtonState, bool middleButtonState, 
                bool rightButtonState,
                int xPosition, int yPosition, int button);
        virtual ~MouseEvent();
       
        NodePtr getElement() const;
        bool getLeftButtonState() const;
        bool getMiddleButtonState() const;
        bool getRightButtonState() const;
        int getXPosition() const;
        int getYPosition() const;
        int getButton() const;

        void setElement(NodePtr pNode);
        virtual void trace();
        
        static const long NO_BUTTON=0;
        static const long LEFT_BUTTON=1;
        static const long RIGHT_BUTTON=2;
        static const long MIDDLE_BUTTON=3;

    private:
        bool m_LeftButtonState;
        bool m_MiddleButtonState;
        bool m_RightButtonState;
        int m_XPosition;
        int m_YPosition;
        int m_Button; // only used in button events
        NodePtr m_pNode;
};

}

#endif

