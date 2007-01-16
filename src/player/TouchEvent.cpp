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
//  Original author of this file is igor@c-base.org
//

#include "TouchEvent.h"

#include "../imaging/ConnectedComps.h"

#include "../graphics/Bitmap.h"
#include "../graphics/Filterfill.h"
#include "../graphics/Pixel8.h"

#include "../base/Logger.h"


namespace avg {
TouchEvent::TouchEvent(int id, Type EventType, BlobInfoPtr info, BlobPtr blob, 
        DPoint& Offset, DPoint& Scale)
    : CursorEvent(id, EventType, transformPoint(info->m_Center, Offset, Scale)),
      m_Info(info),
      m_Blob(blob)
{
}

Event* TouchEvent::cloneAs(Type EventType)
{
    TouchEvent *res = new TouchEvent(*this);
    res->m_Type = EventType;
    return res;
}
#ifdef BROKEN
BitmapPtr TouchEvent::getBitmap() 
{
    IntRect bb = getBoundingBox();
    IntPoint img_size = IntPoint(bb.Width(),bb.Height());
    BitmapPtr res = BitmapPtr(new Bitmap(img_size, I8));
    FilterFill<Pixel8>(0).applyInPlace(res);
    m_Blob->render(res, m_Blob);
    return res;
}
#endif

void TouchEvent::trace()
{
    Event::trace();
    AVG_TRACE(Logger::EVENTS2, "pos: " << m_Position 
            << ", ID: " << getCursorID()
            << ", Area: " << m_Info->m_Area
            << ", Eccentricity: " << m_Info->m_Eccentricity);
}
      
IntPoint TouchEvent::transformPoint(DPoint& pt, DPoint& Offset, DPoint& Scale)
{
    return IntPoint(
            int(round(pt.x*Scale.x)+Offset.x), 
            int(round(pt.y*Scale.y)+Offset.y));
}

}

