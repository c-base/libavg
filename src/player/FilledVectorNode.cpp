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

#include "FilledVectorNode.h"

#include "NodeDefinition.h"
#include "Image.h"
#include "DivNode.h"

#include "../player/SDLDisplayEngine.h"
#include "../base/ScopeTimer.h"
#include "../base/Logger.h"

using namespace std;
using namespace boost;

namespace avg {

NodeDefinition FilledVectorNode::createDefinition()
{
    return NodeDefinition("filledvector")
        .extendDefinition(VectorNode::createDefinition())
        .addArg(Arg<string>("filltexhref", "", false, 
                offsetof(FilledVectorNode, m_FillTexHRef)))
        .addArg(Arg<double>("fillopacity", 0, false, 
                offsetof(FilledVectorNode, m_FillOpacity)))
        .addArg(Arg<string>("fillcolor", "FFFFFF", false, 
                offsetof(FilledVectorNode, m_sFillColorName)))
        ;
}

FilledVectorNode::FilledVectorNode(const ArgList& Args)
    : VectorNode(Args),
      m_FillTexCoord1(0,0),
      m_FillTexCoord2(1,1),
      m_pFillShape(new Shape("", GL_REPEAT, GL_REPEAT))
{
    m_FillTexHRef = Args.getArgVal<string>("filltexhref"); 
    setFillTexHRef(m_FillTexHRef);
    m_sFillColorName = Args.getArgVal<string>("fillcolor");
    m_FillColor = colorStringToColor(m_sFillColorName);
}

FilledVectorNode::~FilledVectorNode()
{
}

void FilledVectorNode::setRenderingEngines(DisplayEngine * pDisplayEngine, 
        AudioEngine * pAudioEngine)
{
    VectorNode::setRenderingEngines(pDisplayEngine, pAudioEngine);
    m_FillColor = colorStringToColor(m_sFillColorName);
    m_pFillShape->moveToGPU(getDisplayEngine());
    m_OldOpacity = -1;
}

void FilledVectorNode::disconnect()
{
    m_pFillShape->moveToCPU();
    VectorNode::disconnect();
}

void FilledVectorNode::checkReload()
{
    Node::checkReload(m_FillTexHRef, m_pFillShape->getImage());
    if (getState() == Node::NS_CANRENDER) {
        m_pFillShape->moveToGPU(getDisplayEngine());
    }
    VectorNode::checkReload();
}

const std::string& FilledVectorNode::getFillTexHRef() const
{
    return m_FillTexHRef;
}

void FilledVectorNode::setFillTexHRef(const string& href)
{
    m_FillTexHRef = href;
    checkReload();
    setDrawNeeded(true);
}

void FilledVectorNode::setFillBitmap(const Bitmap * pBmp)
{
    m_FillTexHRef = "";
    m_pFillShape->setBitmap(pBmp);
    setDrawNeeded(true);
}

const DPoint& FilledVectorNode::getFillTexCoord1() const
{
    return m_FillTexCoord1;
}

void FilledVectorNode::setFillTexCoord1(const DPoint& pt)
{
    m_FillTexCoord1 = pt;
    setDrawNeeded(false);
}

const DPoint& FilledVectorNode::getFillTexCoord2() const
{
    return m_FillTexCoord2;
}

void FilledVectorNode::setFillTexCoord2(const DPoint& pt)
{
    m_FillTexCoord2 = pt;
    setDrawNeeded(false);
}

double FilledVectorNode::getFillOpacity() const
{
    return m_FillOpacity;
}

void FilledVectorNode::setFillOpacity(double opacity)
{
    m_FillOpacity = opacity;
    setDrawNeeded(false);
}

void FilledVectorNode::preRender()
{
    Node::preRender();
    double curOpacity = getParent()->getEffectiveOpacity()*m_FillOpacity;
    VertexArrayPtr pFillVA;
    pFillVA = m_pFillShape->getVertexArray();
    if (hasVASizeChanged()) {
        pFillVA->changeSize(getNumFillVertexes(), getNumFillIndexes());
    }
    if (isDrawNeeded() || curOpacity != m_OldOpacity) {
        pFillVA->reset();
        Pixel32 color = getFillColorVal();
        color.setA((unsigned char)(curOpacity*255));
        calcFillVertexes(pFillVA, color);
        pFillVA->update();
        m_OldOpacity = curOpacity;
    }
    VectorNode::preRender();
}

void FilledVectorNode::maybeRender(const DRect& Rect)
{
    assert(getState() == NS_CANRENDER);
    if (getEffectiveOpacity() > 0.01 || 
            getParent()->getEffectiveOpacity()*m_FillOpacity > 0.01) 
    {
        if (getID() != "") {
            AVG_TRACE(Logger::BLTS, "Rendering " << getTypeStr() << 
                    " with ID " << getID());
        } else {
            AVG_TRACE(Logger::BLTS, "Rendering " << getTypeStr()); 
        }
        getDisplayEngine()->setBlendMode(getBlendMode());
        render(Rect);
    }
}

static ProfilingZone RenderProfilingZone("FilledVectorNode::render");

void FilledVectorNode::render(const DRect& rect)
{
    ScopeTimer Timer(RenderProfilingZone);
    double curOpacity = getParent()->getEffectiveOpacity()*m_FillOpacity;
    glColor4d(1.0, 1.0, 1.0, curOpacity);
    m_pFillShape->draw();
    VectorNode::render(rect);
}

void FilledVectorNode::setFillColor(const string& sColor)
{
    if (m_sFillColorName != sColor) {
        m_sFillColorName = sColor;
        m_FillColor = colorStringToColor(m_sFillColorName);
        setDrawNeeded(false);
    }
}

const string& FilledVectorNode::getFillColor() const
{
    return m_sFillColorName;
}

Pixel32 FilledVectorNode::getFillColorVal() const
{
    return m_FillColor;
}

DPoint FilledVectorNode::calcFillTexCoord(const DPoint& pt, const DPoint& minPt, 
        const DPoint& maxPt)
{
    DPoint texPt;
    texPt.x = (m_FillTexCoord2.x-m_FillTexCoord1.x)*(pt.x-minPt.x)/(maxPt.x-minPt.x)
            +m_FillTexCoord1.x;
    texPt.y = (m_FillTexCoord2.y-m_FillTexCoord1.y)*(pt.y-minPt.y)/(maxPt.y-minPt.y)
            +m_FillTexCoord1.y;
    return texPt;
}

}
