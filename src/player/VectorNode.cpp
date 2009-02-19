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

#include "VectorNode.h"

#include "NodeDefinition.h"
#include "SDLDisplayEngine.h"
#include "OGLSurface.h"
#include "Image.h"

#include "../base/Exception.h"
#include "../base/Logger.h"
#include "../base/ScopeTimer.h"
#include "../base/Exception.h"

#include "../graphics/VertexArray.h"
#include "../graphics/Filterfliprgb.h"

#include <Magick++.h>

#include <iostream>
#include <sstream>

using namespace std;
using namespace boost;

namespace avg {

NodeDefinition VectorNode::createDefinition()
{
    return NodeDefinition("vector")
        .extendDefinition(Node::createDefinition())
        .addArg(Arg<string>("color", "FFFFFF", false, offsetof(VectorNode, m_sColorName)))
        .addArg(Arg<double>("strokewidth", 1, false, offsetof(VectorNode, m_StrokeWidth)))
        .addArg(Arg<string>("texhref", "", false, offsetof(VectorNode, m_TexHRef)))
        ;
}

VectorNode::VectorNode(const ArgList& Args)
    : m_pImage(new Image("", true))
{
    m_TexHRef = Args.getArgVal<string>("texhref"); 
    setTexHRef(m_TexHRef);
}

VectorNode::~VectorNode()
{
}

void VectorNode::setRenderingEngines(DisplayEngine * pDisplayEngine, 
        AudioEngine * pAudioEngine)
{
    setDrawNeeded(true);
    m_Color = colorStringToColor(m_sColorName);
    m_pImage->moveToGPU(dynamic_cast<SDLDisplayEngine*>(pDisplayEngine));
    Node::setRenderingEngines(pDisplayEngine, pAudioEngine);
    if (isTextured()) {
        createTexture();
        downloadTexture();
    }

    m_pVertexArray = VertexArrayPtr(new VertexArray(getNumVertexes(), getNumIndexes(),
            100, 100));
    m_OldOpacity = -1;
}

void VectorNode::connect()
{
    Node::connect();
    checkReload();
}

void VectorNode::disconnect()
{
    m_pVertexArray = VertexArrayPtr();
    m_pImage->moveToCPU();
    deleteTexture();

    Node::disconnect();
}

const std::string& VectorNode::getTexHRef() const
{
    return m_TexHRef;
}

void VectorNode::setTexHRef(const string& href)
{
    m_TexHRef = href;
    checkReload();
}

static ProfilingZone PrerenderProfilingZone("VectorNode::prerender");
static ProfilingZone VAProfilingZone("VectorNode::update VA");
static ProfilingZone VASizeProfilingZone("VectorNode::resize VA");

void VectorNode::preRender()
{
    ScopeTimer Timer(PrerenderProfilingZone);
    double curOpacity = getEffectiveOpacity();

    if (m_bVASizeChanged) {
        ScopeTimer Timer(VASizeProfilingZone);
        m_pVertexArray->changeSize(getNumVertexes(), getNumIndexes());
        m_bVASizeChanged = false;
    }
    {
        ScopeTimer Timer(VAProfilingZone);
        if (m_bDrawNeeded || curOpacity != m_OldOpacity) {
            m_pVertexArray->reset();
            calcVertexes(m_pVertexArray, curOpacity);
            m_pVertexArray->update();
            m_bDrawNeeded = false;
            m_OldOpacity = curOpacity;
        }
    }
    
}

void VectorNode::maybeRender(const DRect& Rect)
{
    assert(getState() == NS_CANRENDER);
    if (getEffectiveOpacity() > 0.01) {
        if (getID() != "") {
            AVG_TRACE(Logger::BLTS, "Rendering " << getTypeStr() << 
                    " with ID " << getID());
        } else {
            AVG_TRACE(Logger::BLTS, "Rendering " << getTypeStr()); 
        }
        render(Rect);
    }
}

static ProfilingZone RenderProfilingZone("VectorNode::render");

void VectorNode::render(const DRect& rect)
{
    ScopeTimer Timer(RenderProfilingZone);
    SDLDisplayEngine * pEngine = getDisplayEngine();
    if (isTextured()) {
        glproc::ActiveTexture(GL_TEXTURE0);
        glBindTexture(pEngine->getTextureMode(), m_TexID);
    }
    pEngine->enableTexture(isTextured());
    pEngine->enableGLColorArray(!isTextured());
    m_pVertexArray->draw();
}

void VectorNode::checkReload()
{
    bool bOldImageExists = isTextured();
    string sLastFilename = m_pImage->getFilename();
    Node::checkReload(m_TexHRef, m_pImage);
    if (m_pImage->getState() == Image::GPU && 
        (sLastFilename != m_pImage->getFilename() || !bOldImageExists))
    {
        createTexture();
        downloadTexture();
    }
}

void VectorNode::setColor(const string& sColor)
{
    if (m_sColorName != sColor) {
        m_sColorName = sColor;
        m_Color = colorStringToColor(m_sColorName);
        m_bDrawNeeded = true;
    }
}

const string& VectorNode::getColor() const
{
    return m_sColorName;
}

void VectorNode::setStrokeWidth(double width)
{
    if (width != m_StrokeWidth) {
        m_bDrawNeeded = true;
        m_StrokeWidth = width;
    }
}

double VectorNode::getStrokeWidth() const
{
    return m_StrokeWidth;
}

Pixel32 VectorNode::getColorVal() const
{
    return m_Color;
}

void VectorNode::updateLineData(VertexArrayPtr& pVertexArray, double opacity,
        const DPoint& p1, const DPoint& p2, double TC1, double TC2)
{
    Pixel32 color = getColorVal();
    color.setA((unsigned char)(opacity*255));

    WideLine wl(p1, p2, getStrokeWidth());
    int curVertex = pVertexArray->getCurVert();
    pVertexArray->appendPos(wl.pl0, calcTexCoord(DPoint(TC1, 1)), color);
    pVertexArray->appendPos(wl.pr0, calcTexCoord(DPoint(TC1, 0)), color);
    pVertexArray->appendPos(wl.pl1, calcTexCoord(DPoint(TC2, 1)), color);
    pVertexArray->appendPos(wl.pr1, calcTexCoord(DPoint(TC2, 0)), color);
    pVertexArray->appendQuadIndexes(curVertex+1, curVertex, curVertex+3, curVertex+2); 
}
     
void VectorNode::setDrawNeeded(bool bSizeChanged)
{
    m_bDrawNeeded = true;
    if (bSizeChanged) {
        m_bVASizeChanged = true;
    }
}
        
DPoint VectorNode::calcTexCoord(const DPoint& origCoord)
{
    SDLDisplayEngine * pEngine = dynamic_cast<SDLDisplayEngine*>(getDisplayEngine());
    if (pEngine->getTextureMode() == GL_TEXTURE_2D || !isTextured()) {
        return origCoord;
    } else {
        DPoint size = DPoint(m_pImage->getSize());
        return origCoord*size;
    }
}

void VectorNode::createTexture()
{
    PixelFormat pf = m_pImage->getPixelFormat();
    IntPoint size = m_pImage->getSize();

    SDLDisplayEngine* pEngine = getDisplayEngine();
    glGenTextures(1, &m_TexID);
    OGLErrorCheck(AVG_ERR_VIDEO_GENERAL, "VectorNode::createTexture: glGenTextures()");
    glproc::ActiveTexture(GL_TEXTURE0);
    int TextureMode = pEngine->getTextureMode();
    glBindTexture(TextureMode, m_TexID);
    OGLErrorCheck(AVG_ERR_VIDEO_GENERAL, "VectorNode::createTexture: glBindTexture()");
    glTexParameteri(TextureMode, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
    glTexParameteri(TextureMode, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
    glTexParameteri(TextureMode, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
    glTexParameteri(TextureMode, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
    OGLErrorCheck(AVG_ERR_VIDEO_GENERAL, 
            "VectorNode::createTexture: glTexParameteri()");
    glPixelStorei(GL_UNPACK_ROW_LENGTH, 0);

    GLenum DestMode = pEngine->getOGLDestMode(pf);
    char * pPixels = 0;
    if (TextureMode == GL_TEXTURE_2D) {
        // Make sure the texture is transparent and black before loading stuff 
        // into it to avoid garbage at the borders.
        int TexMemNeeded = size.x*size.y*Bitmap::getBytesPerPixel(pf);
        pPixels = new char[TexMemNeeded];
        memset(pPixels, 0, TexMemNeeded);
    }
    glTexImage2D(TextureMode, 0, DestMode, size.x, size.y, 0,
            pEngine->getOGLSrcMode(pf), pEngine->getOGLPixelType(pf), pPixels);
    OGLErrorCheck(AVG_ERR_VIDEO_GENERAL, 
            "VectorNode::createTexture: glTexImage2D()");
    if (TextureMode == GL_TEXTURE_2D) {
        free(pPixels);
    }
}

void VectorNode::downloadTexture() const
{
    SDLDisplayEngine* pEngine = getDisplayEngine();
    OGLSurface* pSurface = m_pImage->getSurface();
    PixelFormat pf = m_pImage->getPixelFormat();
    IntPoint size = m_pImage->getSize();
    pSurface->bindPBO();
    int TextureMode = pEngine->getTextureMode();
    glBindTexture(TextureMode, m_TexID);
    OGLErrorCheck(AVG_ERR_VIDEO_GENERAL, 
            "VectorNode::downloadTexture: glBindTexture()");
    glPixelStorei(GL_UNPACK_ROW_LENGTH, 0);
    glTexSubImage2D(TextureMode, 0, 0, 0, size.x, size.y,
            pEngine->getOGLSrcMode(pf), pEngine->getOGLPixelType(pf), 0);
    OGLErrorCheck(AVG_ERR_VIDEO_GENERAL, 
            "VectorNode::downloadTexture: glTexSubImage2D()");
    pSurface->unbindPBO();
}

void VectorNode::deleteTexture()
{
    glDeleteTextures(1, &m_TexID);
}

bool VectorNode::isTextured() const
{
    return (m_pImage->getState() != Image::NOT_AVAILABLE);
}



WideLine::WideLine(const DPoint& p0, const DPoint& p1, double width)
    : pt0(p0),
      pt1(p1)
{
    DPoint m = (pt1-pt0);
    m.normalize();
    DPoint w = DPoint(m.y, -m.x)*width/2;
    pl0 = p0-w;
    pr0 = p0+w;
    pl1 = p1-w;
    pr1 = p1+w;
    dir = DPoint(w.y, -w.x); 
}

std::ostream& operator<<(std::ostream& os, const WideLine& line)
{
    os << "(" << line.pt0 << "," << line.pt1 << ")";
    return os;
}


}
