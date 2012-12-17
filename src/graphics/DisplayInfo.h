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

#ifndef _DisplayInfo_H_
#define _DisplayInfo_H_

#include "../api.h"
#include "../avgconfigwrapper.h"

#include "../base/GLMHelper.h"

#include <boost/shared_ptr.hpp>

namespace avg {

class AVG_API DisplayInfo
{
public:
    DisplayInfo(float ppmm);
    virtual ~DisplayInfo();
 
    IntPoint getScreenResolution();
    float getPixelsPerMM();
    void assumePixelsPerMM(float ppmm);
    glm::vec2 getPhysicalScreenDimensions();
    
private:
    void calcScreenDimensions(float ppmm=0);

    IntPoint m_ScreenResolution;
    float m_PPMM;
};

typedef boost::shared_ptr<DisplayInfo> DisplayInfoPtr;

}
 
#endif
