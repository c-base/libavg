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

#include "GPUShadowFilter.h"
#include "Bitmap.h"
#include "ShaderRegistry.h"
#include "ImagingProjection.h"

#include "../base/ObjectCounter.h"
#include "../base/MathHelper.h"
#include "../base/Exception.h"

#include <string.h>
#include <iostream>

#define SHADERID_HORIZ "horizshadow"
#define SHADERID_VERT "vertshadow"

using namespace std;

namespace avg {

GPUShadowFilter::GPUShadowFilter(const IntPoint& size, const glm::vec2& offset, 
        float stdDev, float opacity, const Pixel32& color)
    : GPUFilter(B8G8R8A8, B8G8R8A8, false, 2)
{
    ObjectCounter::get()->incRef(&typeid(*this));

    setDimensions(size, stdDev, offset);
    createShader(SHADERID_HORIZ);
    createShader(SHADERID_VERT);
    setParams(offset, stdDev, opacity, color);
}

GPUShadowFilter::~GPUShadowFilter()
{
    ObjectCounter::get()->decRef(&typeid(*this));
}

void GPUShadowFilter::setParams(const glm::vec2& offset, float stdDev, float opacity, 
        const Pixel32& color)
{
    m_Offset = offset;
    m_StdDev = stdDev;
    m_Opacity = opacity;
    m_Color = color;
    m_pGaussCurveTex = calcBlurKernelTex(m_StdDev, m_Opacity, false);
    setDimensions(getSrcSize(), stdDev, offset);
    IntRect destRect2(IntPoint(0,0), getDestRect().size());
    m_pProjection2 = ImagingProjectionPtr(new ImagingProjection(
            getDestRect().size(), destRect2));
}

void GPUShadowFilter::applyOnGPU(GLTexturePtr pSrcTex)
{
    int kernelWidth = m_pGaussCurveTex->getSize().x;
    getFBO(1)->activate();
    OGLShaderPtr pHShader = getShader(SHADERID_HORIZ);
    pHShader->activate();
    pHShader->setUniformFloatParam("width", float(kernelWidth));
    pHShader->setUniformIntParam("radius", (kernelWidth-1)/2);
    pHShader->setUniformIntParam("texture", 0);
    pHShader->setUniformIntParam("kernelTex", 1);
    IntPoint size = getSrcSize();
    glm::vec2 texOffset(m_Offset.x/size.x, m_Offset.y/size.y);
    pHShader->setUniformVec2fParam("offset", texOffset);
    m_pGaussCurveTex->activate(GL_TEXTURE1);
    draw(pSrcTex);

    m_pProjection2->activate();
    getFBO(0)->activate();
    OGLShaderPtr pVShader = getShader(SHADERID_VERT);
    pVShader->activate();
    pVShader->setUniformFloatParam("width", float(kernelWidth));
    pVShader->setUniformIntParam("radius", (kernelWidth-1)/2);
    pVShader->setUniformIntParam("hBlurTex", 0);
    pVShader->setUniformIntParam("kernelTex", 1);
    pVShader->setUniformColorParam("color", m_Color);

    pSrcTex->activate(GL_TEXTURE2);
    pVShader->setUniformIntParam("origTex", 2);
    FRect destRect = getRelDestRect();
    pVShader->setUniformVec2fParam("destPos", destRect.tl);
    pVShader->setUniformVec2fParam("destSize", destRect.size());
    getDestTex(1)->activate(GL_TEXTURE0);
    m_pProjection2->draw();
    glproc::UseProgramObject(0);
}

void GPUShadowFilter::setDimensions(IntPoint size, float stdDev, const glm::vec2& offset)
{
    int radius = getBlurKernelRadius(stdDev);
    IntPoint radiusOffset(radius, radius);
    IntPoint intOffset(offset);
    IntRect destRect(intOffset-radiusOffset, intOffset+size+radiusOffset+IntPoint(1,1));
    destRect.expand(IntRect(IntPoint(0,0), size));
    GPUFilter::setDimensions(size, destRect, GL_CLAMP_TO_BORDER);
}
 
}
