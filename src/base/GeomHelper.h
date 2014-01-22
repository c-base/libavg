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
//

#ifndef _GeomHelper_H_
#define _GeomHelper_H_

#include "../api.h"

#include "../glm/glm.hpp"

#include <vector>

namespace avg {

struct AVG_API LineSegment {
public:
    LineSegment(const glm::vec2& pt0, const glm::vec2& pt1);
    glm::vec2 p0;
    glm::vec2 p1;

    bool isPointOver(const glm::vec2& pt);
};

bool AVG_API lineSegmentsIntersect(const LineSegment& l0, const LineSegment& l1);

bool AVG_API pointInPolygon(const glm::vec2& pt, const std::vector<glm::vec2>& poly); 

glm::vec2 AVG_API getLineLineIntersection(const glm::vec2& p1, const glm::vec2& v1, 
        const glm::vec2& p2, const glm::vec2& v2);

}
#endif
 
