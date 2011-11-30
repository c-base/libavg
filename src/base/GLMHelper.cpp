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

#include "GLMHelper.h"
#include "StringHelper.h"
#include "MathHelper.h"

#include "../glm/gtx/rotate_vector.hpp"

using namespace std;

namespace avg {

glm::vec2 getRotated(const glm::vec2& vec, float angle)
{
    return glm::rotate(vec, angle*180/PI);
}

glm::vec2 getRotatedPivot(const glm::vec2& vec, float angle, const glm::vec2& pivot)
{
    // translate pivot to origin
    glm::vec2 translated = vec - pivot;
   
    // calculate rotated coordinates about the origin
    glm::vec2 rotated = glm::rotate(translated, angle*180/PI);

    // re-translate pivot to original position
    rotated += pivot;

    return rotated;
}

float getAngle(const glm::vec2& vec)
{
    return float(atan2(double(vec.y), double(vec.x)));
}

glm::vec2 fromPolar(float angle, float radius)
{
    return glm::vec2(cos(angle)*radius, sin(angle)*radius);
}

template<class NUM>
bool almostEqual(const glm::detail::tvec2<NUM>& v1, const glm::detail::tvec2<NUM>& v2)
{
    return (fabs(v1.x-v2.x)+fabs(v1.y-v2.y)) < 0.0001;
}

template<class NUM>
std::ostream& operator<<( std::ostream& os, const glm::detail::tvec2<NUM> &v)
{
    os << "(" << v.x << "," << v.y << ")";
    return os;
}

template<class NUM>
std::istream& operator>>(std::istream& is, glm::detail::tvec2<NUM>& p)
{
    skipToken(is, '(');
    is >> p.x;
    skipToken(is, ',');
    is >> p.y;
    skipToken(is, ')');
    return is;
}

glm::vec2 stringToVec2(const std::string& s)
{
    glm::vec2 pt;
    fromString(s, pt);
    return pt;
}


template std::ostream& operator<<(std::ostream& os, const glm::detail::tvec2<int> &p);
template std::ostream& operator<<(std::ostream& os, const glm::detail::tvec2<float> &p);
template std::ostream& operator<<(std::ostream& os, const glm::detail::tvec2<double> &p);

template std::istream& operator>>(std::istream& is, glm::detail::tvec2<int>& p);
template std::istream& operator>>(std::istream& is, glm::detail::tvec2<float>& p);
template std::istream& operator>>(std::istream& is, glm::detail::tvec2<double>& p);

template bool almostEqual(const glm::detail::tvec2<float>& v1,
        const glm::detail::tvec2<float>& v2);
template bool almostEqual(const glm::detail::tvec2<double>& v1,
        const glm::detail::tvec2<double>& v2);
}

