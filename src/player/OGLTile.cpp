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

#include "OGLTile.h"
#include "SDLDisplayEngine.h"
#include "../base/Logger.h"
#include "../base/Exception.h"
#include "../base/ScopeTimer.h"
#include "../base/ObjectCounter.h"

#include <iostream>
#include <string>

namespace avg {

using namespace std;
    
OGLTile::OGLTile(IntRect Extent, IntPoint TexSize, int Stride, PixelFormat pf, 
        SDLDisplayEngine * pEngine) 
    : m_Extent(Extent),
      m_TexSize(TexSize),
      m_pf(pf),
      m_pEngine(pEngine)
{
    ObjectCounter::get()->incRef(&typeid(*this));
    if (m_pf == YCbCr420p || m_pf == YCbCrJ420p) {
        createTexture(0, m_TexSize, Stride, I8);
        createTexture(1, m_TexSize/2, Stride/2, I8);
        createTexture(2, m_TexSize/2, Stride/2, I8);
    } else {
        createTexture(0, m_TexSize, Stride, m_pf);
    }
}

OGLTile::~OGLTile()
{
    if (m_pf == YCbCr420p || m_pf == YCbCrJ420p) {
        glDeleteTextures(3, m_TexID);
    } else {
        glDeleteTextures(1, m_TexID);
    }
    OGLErrorCheck(AVG_ERR_VIDEO_GENERAL, "OGLTile::~OGLTile: glDeleteTextures()");    
    ObjectCounter::get()->decRef(&typeid(*this));
}

const IntRect& OGLTile::getExtent() const
{
    return m_Extent;
}

const IntPoint& OGLTile::getTexSize() const
{
    return m_TexSize;
}

int OGLTile::getTexID(int i) const
{
    return m_TexID[i];
}

void OGLTile::blt(const DPoint& TLPoint, const DPoint& TRPoint,
        const DPoint& BLPoint, const DPoint& BRPoint) const
{
    double TexWidth;
    double TexHeight;
    int TextureMode = m_pEngine->getTextureMode();
    
    if (TextureMode == GL_TEXTURE_2D) {
        TexWidth = double(m_Extent.Width())/m_TexSize.x;
        TexHeight = double(m_Extent.Height())/m_TexSize.y;
    } else {
        TexWidth = m_TexSize.x;
        TexHeight = m_TexSize.y;
    }
    
    if (m_pf == YCbCr420p || m_pf == YCbCrJ420p) {
        GLhandleARB hProgram;
        if (m_pf == YCbCr420p) {
            hProgram = m_pEngine->getYCbCr420pShader()->getProgram();
        } else {
            hProgram = m_pEngine->getYCbCrJ420pShader()->getProgram();
        }
        glproc::UseProgramObject(hProgram);
        OGLErrorCheck(AVG_ERR_VIDEO_GENERAL, "OGLTile::blt: glUseProgramObject()");
        glproc::ActiveTexture(GL_TEXTURE0);
        glBindTexture(TextureMode, m_TexID[0]);
        glproc::Uniform1i(glproc::GetUniformLocation(hProgram, "YTexture"), 0);
        glproc::ActiveTexture(GL_TEXTURE1);
        glBindTexture(TextureMode, m_TexID[1]);
        glproc::Uniform1i(glproc::GetUniformLocation(hProgram, "CbTexture"), 1);
        glproc::ActiveTexture(GL_TEXTURE2);
        glBindTexture(TextureMode, m_TexID[2]);
        glproc::Uniform1i(glproc::GetUniformLocation(hProgram, "CrTexture"), 2);
        OGLErrorCheck(AVG_ERR_VIDEO_GENERAL, "OGLTile::blt: glUniform1i()");
    } else {
        glproc::ActiveTexture(GL_TEXTURE0);
        glBindTexture(TextureMode, m_TexID[0]);
        if (m_pEngine->getYCbCrMode() == OGL_SHADER) {
            glproc::UseProgramObject(0);
        }
    }
    glBegin(GL_QUADS);
    glTexCoord2d(0.0, 0.0);
    glVertex3d (TLPoint.x, TLPoint.y, 0.0);
    glTexCoord2d(TexWidth, 0.0);
    glVertex3d (TRPoint.x, TRPoint.y, 0.0);
    glTexCoord2d(TexWidth, TexHeight);
    glVertex3d (BRPoint.x, BRPoint.y, 0.0);
    glTexCoord2d(0.0, TexHeight);
    glVertex3d (BLPoint.x, BLPoint.y, 0.0);
    glEnd();
    OGLErrorCheck(AVG_ERR_VIDEO_GENERAL, "OGLTile::blt: glEnd()");
    if (m_pf == YCbCr420p || m_pf == YCbCrJ420p) {
        glproc::ActiveTexture(GL_TEXTURE1);
        glDisable(TextureMode);
        glproc::ActiveTexture(GL_TEXTURE2);
        glDisable(TextureMode);
        glproc::ActiveTexture(GL_TEXTURE0);
        glproc::UseProgramObject(0);
        OGLErrorCheck(AVG_ERR_VIDEO_GENERAL, "OGLTile::blt: glDisable(TextureMode)");
    }
}

void OGLTile::createTexture(int i, IntPoint Size, int Stride, PixelFormat pf)
{
    glGenTextures(1, &m_TexID[i]);
    OGLErrorCheck(AVG_ERR_VIDEO_GENERAL, "OGLTile::createTexture: glGenTextures()");
    glproc::ActiveTexture(GL_TEXTURE0+i);
    int TextureMode = m_pEngine->getTextureMode();
    glBindTexture(TextureMode, m_TexID[i]);
    OGLErrorCheck(AVG_ERR_VIDEO_GENERAL, "OGLTile::createTexture: glBindTexture()");
    glTexParameteri(TextureMode, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
    glTexParameteri(TextureMode, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
    glTexParameteri(TextureMode, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
    glTexParameteri(TextureMode, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
    OGLErrorCheck(AVG_ERR_VIDEO_GENERAL, 
            "OGLTile::createTexture: glTexParameteri()");
    glPixelStorei(GL_UNPACK_ROW_LENGTH, Size.x);
    OGLErrorCheck(AVG_ERR_VIDEO_GENERAL, 
            "OGLTile::createTexture: glPixelStorei(GL_UNPACK_ROW_LENGTH)");
    
    GLenum DestMode = m_pEngine->getOGLDestMode(pf);
#if defined(__APPLE__) && !defined(__i386__)
    // XXX: Hack to work around broken Mac OS X GL_ALPHA/GL_UNPACK_ROW_LENGTH on 
    // PPC macs. If this is gone, the Stride parameter can be removed too :-).
    if (Stride != Size.x && DestMode == GL_ALPHA &&
            (m_pf == YCbCr420p || m_pf == YCbCrJ420p)) 
    {
        DestMode = GL_RGBA;
    }
#endif
    char * pPixels = 0;
    if (TextureMode == GL_TEXTURE_2D) {
        // Make sure the texture is transparent and black before loading stuff 
        // into it to avoid garbage at the borders.
        int TexMemNeeded = Size.x*Size.y*Bitmap::getBytesPerPixel(pf);
        pPixels = new char[TexMemNeeded];
        memset(pPixels, 0, TexMemNeeded);
    }
    glTexImage2D(TextureMode, 0, DestMode, Size.x, Size.y, 0,
            m_pEngine->getOGLSrcMode(pf), m_pEngine->getOGLPixelType(pf), pPixels);
    OGLErrorCheck(AVG_ERR_VIDEO_GENERAL, 
            "OGLTile::createTexture: glTexImage2D()");
    if (TextureMode == GL_TEXTURE_2D) {
        free(pPixels);
    }
}

static ProfilingZone TexSubImageProfilingZone("OGLTile::texture download");

void OGLTile::downloadTexture(int i, BitmapPtr pBmp, int stride, 
                OGLMemoryMode MemoryMode) const
{
    PixelFormat pf;
    if (m_pf == YCbCr420p || m_pf == YCbCrJ420p) {
        pf = I8;
    } else {
        pf = m_pf;
    }
    IntRect Extent = m_Extent;
    if (i != 0) {
        stride /= 2;
        Extent = IntRect(m_Extent.tl/2.0, m_Extent.br/2.0);
    }
    int TextureMode = m_pEngine->getTextureMode();
    glBindTexture(TextureMode, m_TexID[i]);
    OGLErrorCheck(AVG_ERR_VIDEO_GENERAL, 
            "OGLTile::downloadTexture: glBindTexture()");
    int bpp = Bitmap::getBytesPerPixel(pf);
    glPixelStorei(GL_UNPACK_ROW_LENGTH, stride);
    OGLErrorCheck(AVG_ERR_VIDEO_GENERAL, 
            "OGLTile::downloadTexture: glPixelStorei(GL_UNPACK_ROW_LENGTH)");
//    cerr << "OGLTile::downloadTexture(" << pBmp << ", stride=" << stride 
//        << ", Extent= " << m_Extent << ", pf= " << Bitmap::getPixelFormatString(m_pf)
//        << ", bpp= " << bpp << endl;
    unsigned char * pStartPos = (unsigned char *)
            (ptrdiff_t)(Extent.tl.y*stride*bpp + Extent.tl.x*bpp);
    if (MemoryMode == OGL) {
        pStartPos += (ptrdiff_t)(pBmp->getPixels());
    }
#ifdef __APPLE__
    // Under Mac OS X 10.5.0 and 10.5.1, the combination of glTexSubImage2D, 
    // GL_ALPHA and PBO is broken if pStartPos is 0. So we use an offset. 
    // There's corresponding code in OGLSurface that undoes this... bleagh.
    if (MemoryMode == PBO && 
            (m_pf == YCbCr420p || m_pf == YCbCrJ420p || m_pf == YCbCr422)) 
    {
        pStartPos += 4;
    }
#endif
    {
        ScopeTimer Timer(TexSubImageProfilingZone);
        glTexSubImage2D(TextureMode, 0, 0, 0, Extent.Width(), Extent.Height(),
                m_pEngine->getOGLSrcMode(pf), m_pEngine->getOGLPixelType(pf), 
                pStartPos);
    }
    OGLErrorCheck(AVG_ERR_VIDEO_GENERAL, 
            "OGLTile::downloadTexture: glTexSubImage2D()");
    
}

}
