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

#ifndef _PolygonNode_H_
#define _PolygonNode_H_

#include "../api.h"
#include "FilledVectorNode.h"

#include "../graphics/Pixel32.h"
#include "../base/WideLine.h"

#include <vector>

namespace avg {

class AVG_API PolygonNode : public FilledVectorNode
{
    public:
        static NodeDefinition createDefinition();
        
        PolygonNode(const ArgList& Args);
        virtual ~PolygonNode();

        const std::vector<DPoint>& getPos() const;
        void setPos(const std::vector<DPoint>& pts);

        const std::vector<double>& getTexCoords() const;
        void setTexCoords(const std::vector<double>& coords);

        std::string getLineJoin() const;
        void setLineJoin(const std::string& s);

        NodePtr getElementByPos(const DPoint & pos);

        virtual void calcVertexes(VertexArrayPtr& pVertexArray, Pixel32 color);
        virtual void calcFillVertexes(VertexArrayPtr& pVertexArray, Pixel32 color);

    private:
        std::vector<DPoint> m_Pts;
        std::vector<double> m_CumulDist;
        std::vector<double> m_TexCoords;
        std::vector<double> m_EffTexCoords;
        LineJoin m_LineJoin;
};

}

#endif

