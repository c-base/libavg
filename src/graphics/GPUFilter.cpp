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

#include "GPUFilter.h"
#include "Bitmap.h"

#include "VertexArray.h"
#include "ImagingProjection.h"
#include "../base/ObjectCounter.h"
#include "../base/Exception.h"
#include "../base/MathHelper.h"

#include <iostream>
#include <string.h>

using namespace std;
using namespace boost;

namespace avg {

thread_specific_ptr<PBOPtr> GPUFilter::s_pFilterKernelPBO;

GPUFilter::GPUFilter(PixelFormat pfSrc, PixelFormat pfDest, bool bStandalone, 
        unsigned numTextures)
    : m_PFSrc(pfSrc),
      m_PFDest(pfDest),
      m_bStandalone(bStandalone),
      m_NumTextures(numTextures),
      m_SrcSize(0,0),
      m_DestRect(0,0,0,0)
{
    ObjectCounter::get()->incRef(&typeid(*this));
}

GPUFilter::~GPUFilter()
{
    ObjectCounter::get()->decRef(&typeid(*this));
}

void GPUFilter::setDimensions(const IntPoint& srcSize)
{
    setDimensions(srcSize, IntRect(IntPoint(0,0), srcSize), GL_CLAMP_TO_EDGE);
}

void GPUFilter::setDimensions(const IntPoint& srcSize, const IntRect& destRect,
        unsigned texMode)
{
    bool bProjectionChanged = false;
    if (destRect != m_DestRect) {
        m_pFBO = FBOPtr(new FBO(destRect.size(), m_PFDest, m_NumTextures));
        m_DestRect = destRect;
        bProjectionChanged = true;
    }
    if (m_bStandalone && srcSize != m_SrcSize) {
        m_pSrcTex = GLTexturePtr(new GLTexture(srcSize, m_PFSrc, false, texMode, 
                texMode));
        m_pSrcPBO = PBOPtr(new PBO(srcSize, m_PFSrc, GL_STREAM_DRAW));
        bProjectionChanged = true;
    }
    m_SrcSize = srcSize;
    if (bProjectionChanged) {
        m_pProjection = ImagingProjectionPtr(new ImagingProjection);
        m_pProjection->setup(srcSize, destRect);
    }
}
  
BitmapPtr GPUFilter::apply(BitmapPtr pBmpSource)
{
    AVG_ASSERT(m_pSrcTex);
    AVG_ASSERT(m_pFBO);
    m_pSrcPBO->moveBmpToTexture(pBmpSource, m_pSrcTex);
    apply(m_pSrcTex);
    BitmapPtr pFilteredBmp = m_pFBO->getImage();
    BitmapPtr pDestBmp;
    if (pFilteredBmp->getPixelFormat() != pBmpSource->getPixelFormat()) {
        pDestBmp = BitmapPtr(new Bitmap(m_DestRect.size(),
                pBmpSource->getPixelFormat()));
        pDestBmp->copyPixels(*pFilteredBmp);
    } else {
        pDestBmp = pFilteredBmp;
    }
    return pDestBmp;
}

void GPUFilter::apply(GLTexturePtr pSrcTex)
{
    m_pFBO->activate();
    m_pProjection->activate();
    applyOnGPU(pSrcTex);
    m_pFBO->deactivate();
    m_pFBO->copyToDestTexture();
}

GLTexturePtr GPUFilter::getDestTex(int i) const
{
    return m_pFBO->getTex(i);
}

BitmapPtr GPUFilter::getImage() const
{
    return m_pFBO->getImage();
}

FBOPtr GPUFilter::getFBO()
{
    return m_pFBO;
}

const IntRect& GPUFilter::getDestRect() const
{
    return m_DestRect;
}

const IntPoint& GPUFilter::getSrcSize() const
{
    return m_SrcSize;
}

DRect GPUFilter::getRelDestRect() const
{
    DPoint srcSize(m_SrcSize);
    return DRect(m_DestRect.tl.x/srcSize.x, m_DestRect.tl.y/srcSize.y,
            m_DestRect.br.x/srcSize.x, m_DestRect.br.y/srcSize.y);
}

void GPUFilter::glContextGone()
{
    s_pFilterKernelPBO.reset();
}

void GPUFilter::draw(GLTexturePtr pTex)
{
    pTex->activate(GL_TEXTURE0);
    m_pProjection->draw();
}

const string& GPUFilter::getStdShaderCode() const
{
    static string sCode = 
        "void unPreMultiplyAlpha(inout vec4 color)\n"
        "{\n"
        "    color.rgb /= color.a;\n"
        "}\n"
        "\n"
        "void preMultiplyAlpha(inout vec4 color)\n"
        "{\n"
        "    color.rgb *= color.a;\n"
        "}\n"
        "\n";

    return sCode;
}

void dumpKernel(int width, float* pKernel)
{
    cerr << "  Kernel width: " << width << endl;
    float sum = 0;
    for (int i = 0; i < width; ++i) {
        sum += pKernel[i];
        cerr << "  " << pKernel[i] << endl;
    }
    cerr << "Sum of coefficients: " << sum << endl;
}

int GPUFilter::getBlurKernelRadius(double stdDev) const
{
    return int(ceil(stdDev*3));
}

GLTexturePtr GPUFilter::calcBlurKernelTex(double stdDev, double opacity) const
// If opacity is -1, this is a brightness-preserving blur.
// Otherwise, opacity is the coefficient of the center pixel.
{
    int kernelCenter = int(ceil(stdDev*3));
    int kernelWidth = kernelCenter*2+1;
    float* pKernel;
    pKernel = new float[kernelWidth];
    float sum = 0;
    for (int i = 0; i <= kernelCenter; ++i) {
        pKernel[kernelCenter+i] = float(exp(-i*i/(2*stdDev*stdDev))
                /sqrt(2*PI*stdDev*stdDev));
        sum += pKernel[kernelCenter+i];
        if (i != 0) {
            pKernel[kernelCenter-i] = pKernel[kernelCenter+i];
            sum += pKernel[kernelCenter-i];
        }
    }

    if (opacity == -1) {
        // This is a brightness-preserving blur.
        // Make sure the sum of coefficients is 1 despite the inaccuracies
        // introduced by using a kernel of finite size.
        for (int i = 0; i < kernelWidth; ++i) {
            pKernel[i] /= sum;
        }
    } else {
        float factor = float(opacity/pKernel[kernelCenter]);
        for (int i = 0; i < kernelWidth; ++i) {
            pKernel[i] *= factor;
        }
    }
//    dumpKernel(kernelWidth, pKernel);
    
    IntPoint size(kernelWidth, 1);
    GLTexturePtr pTex(new GLTexture(size, R32G32B32A32F));
    if (s_pFilterKernelPBO.get() == 0) {
        s_pFilterKernelPBO.reset(new PBOPtr(new PBO(IntPoint(1024, 1), R32G32B32A32F,
                GL_STREAM_DRAW)));
    }
    (*s_pFilterKernelPBO)->activate();
    void * pPBOPixels = glproc::MapBuffer(GL_PIXEL_UNPACK_BUFFER_EXT, GL_WRITE_ONLY);
    OGLErrorCheck(AVG_ERR_VIDEO_GENERAL, "GPUFilter::calcBlurKernelTex MapBuffer()");
    float * pCurFloat = (float*)pPBOPixels;
    for (int i = 0; i < kernelWidth; ++i) {
        for (int j = 0; j < 4; ++j) {
            *pCurFloat = pKernel[i];
            ++pCurFloat;
        }
    }
    glproc::UnmapBuffer(GL_PIXEL_UNPACK_BUFFER_EXT);
    OGLErrorCheck(AVG_ERR_VIDEO_GENERAL, "GPUFilter::calcBlurKernelTex UnmapBuffer()");
   
    (*s_pFilterKernelPBO)->movePBOToTexture(pTex);

    delete[] pKernel;
    return pTex;
}

}
