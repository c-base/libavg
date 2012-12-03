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

#include "PBO.h"
#include "GLContext.h"

#include "../base/Logger.h"
#include "../base/Exception.h"
#include "../base/ObjectCounter.h"

#include <iostream>
#include <cstring>

using namespace std;
using namespace boost;

namespace avg {
    
PBO::PBO(const IntPoint& size, PixelFormat pf, unsigned usage)
    : TextureMover(size, pf),
      m_Usage(usage)
{
    ObjectCounter::get()->incRef(&typeid(*this));
    m_PBOID = GLContext::getCurrent()->getPBOCache().getBuffer();
    
    unsigned target = getTarget();
    glproc::BindBuffer(target, m_PBOID);
    GLContext::checkError("PBO: BindBuffer()");
    glproc::BufferData(target, getMemNeeded(), 0, usage);
    GLContext::checkError("PBO: BufferData()");
    glproc::BindBuffer(target, 0);
}

PBO::~PBO()
{
    glproc::BindBuffer(getTarget(), m_PBOID);
    glproc::BufferData(getTarget(), 0, 0, m_Usage);
    GLContext* pContext = GLContext::getCurrent();
    if (pContext) {
        pContext->getPBOCache().returnBuffer(m_PBOID);
    }
    glproc::BindBuffer(getTarget(), 0);
    GLContext::checkError("PBO: DeleteBuffers()");
    ObjectCounter::get()->decRef(&typeid(*this));
}

void PBO::activate()
{
    glproc::BindBuffer(getTarget(), m_PBOID);
    GLContext::checkError("PBO::activate()");  
}

int PBO::getID() const
{
    return m_PBOID;
}

void PBO::moveBmpToTexture(BitmapPtr pBmp, GLTexture& tex)
{
    AVG_ASSERT(pBmp->getSize() == tex.getSize());
    AVG_ASSERT(getSize() == pBmp->getSize());
    AVG_ASSERT(pBmp->getPixelFormat() == getPF());
    AVG_ASSERT(tex.getPF() == getPF());
    AVG_ASSERT(!isReadPBO());
    glproc::BindBuffer(GL_PIXEL_UNPACK_BUFFER_EXT, m_PBOID);
    GLContext::checkError("PBO::moveBmpToTexture BindBuffer()");
    void * pPBOPixels = glproc::MapBuffer(GL_PIXEL_UNPACK_BUFFER_EXT, GL_WRITE_ONLY);
    GLContext::checkError("PBO::moveBmpToTexture MapBuffer()");
    Bitmap PBOBitmap(getSize(), getPF(), (unsigned char *)pPBOPixels, getStride(), false);
    PBOBitmap.copyPixels(*pBmp);
    glproc::UnmapBuffer(GL_PIXEL_UNPACK_BUFFER_EXT);
    GLContext::checkError("PBO::setImage: UnmapBuffer()");

    tex.setDirty();
    moveToTexture(tex);
}

BitmapPtr PBO::moveTextureToBmp(GLTexture& tex, int mipmapLevel)
{
    moveTextureToPBO(tex, mipmapLevel); 
    return movePBOToBmp();
}

void PBO::moveTextureToPBO(GLTexture& tex, int mipmapLevel)
{
    AVG_ASSERT(isReadPBO());
    AVG_ASSERT(getSize() == tex.getGLSize());
    AVG_ASSERT(getPF() == tex.getPF());
    glproc::BindBuffer(GL_PIXEL_PACK_BUFFER_EXT, m_PBOID);
    GLContext::checkError("PBO::getImage BindBuffer()");

    tex.activate(GL_TEXTURE0);

    glGetTexImage(GL_TEXTURE_2D, mipmapLevel, GLTexture::getGLFormat(getPF()), 
            GLTexture::getGLType(getPF()), 0);
    GLContext::checkError("PBO::getImage: glGetTexImage()");
    if (mipmapLevel == 0) {
        m_ActiveSize = tex.getSize();
        m_BufferStride = tex.getGLSize().x;
    } else {
        m_ActiveSize = tex.getMipmapSize(mipmapLevel);
        m_BufferStride = tex.getMipmapSize(mipmapLevel).x;
    }
}

BitmapPtr PBO::movePBOToBmp() const
{
    AVG_ASSERT(isReadPBO());
    glproc::BindBuffer(GL_PIXEL_PACK_BUFFER_EXT, m_PBOID);
    GLContext::checkError("PBO::getImage BindBuffer()");
    void * pPBOPixels = glproc::MapBuffer(GL_PIXEL_PACK_BUFFER_EXT, GL_READ_ONLY);
    GLContext::checkError("PBO::getImage MapBuffer()");
    Bitmap PBOBitmap(m_ActiveSize, getPF(), (unsigned char *)pPBOPixels, 
            m_BufferStride*getBytesPerPixel(getPF()), false);
    BitmapPtr pBmp(new Bitmap(m_ActiveSize, getPF()));
    pBmp->copyPixels(PBOBitmap);
    glproc::UnmapBuffer(GL_PIXEL_PACK_BUFFER_EXT);
    GLContext::checkError("PBO::getImage: UnmapBuffer()");
    glproc::BindBuffer(GL_PIXEL_PACK_BUFFER_EXT, 0);
    
    return pBmp;
}

BitmapPtr PBO::lock()
{
    AVG_ASSERT(!isReadPBO());
    BitmapPtr pBmp;
    glproc::BindBuffer(GL_PIXEL_UNPACK_BUFFER_EXT, m_PBOID);
    GLContext::checkError("PBOTexture::lockBmp: glBindBuffer()");
    glproc::BufferData(GL_PIXEL_UNPACK_BUFFER_EXT, getMemNeeded(), 0, m_Usage);
    GLContext::checkError("PBOTexture::lockBmp: glBufferData()");
    unsigned char * pBuffer = (unsigned char *)
        glproc::MapBuffer(GL_PIXEL_UNPACK_BUFFER_EXT, GL_WRITE_ONLY);
    GLContext::checkError("PBOTexture::lockBmp: glMapBuffer()");
    glproc::BindBuffer(GL_PIXEL_UNPACK_BUFFER_EXT, 0);
    GLContext::checkError("PBOTexture::lockBmp: glBindBuffer(0)");

    pBmp = BitmapPtr(new Bitmap(getSize(), getPF(), pBuffer, getStride(), false));
    return pBmp;
}

void PBO::unlock()
{
    AVG_ASSERT(!isReadPBO());
    glproc::BindBuffer(GL_PIXEL_UNPACK_BUFFER_EXT, m_PBOID);
    GLContext::checkError("PBOTexture::unlockBmp: glBindBuffer()");
    glproc::UnmapBuffer(GL_PIXEL_UNPACK_BUFFER_EXT);
    GLContext::checkError("PBOTexture::unlockBmp: glUnmapBuffer()");
    glproc::BindBuffer(GL_PIXEL_UNPACK_BUFFER_EXT, 0);
    GLContext::checkError("PBOTexture::unlockBmp: glBindBuffer(0)");
}

void PBO::moveToTexture(GLTexture& tex)
{
    AVG_ASSERT(!isReadPBO());
    IntPoint size = tex.getSize();
    if (size.x > getSize().x) {
        size.x = getSize().x;
    } 
    if (size.y > getSize().y) {
        size.y = getSize().y;
    } 
    glproc::BindBuffer(GL_PIXEL_UNPACK_BUFFER_EXT, m_PBOID);
    GLContext::checkError("PBOTexture::lockBmp: glBindBuffer()");
    tex.activate(GL_TEXTURE0);
#ifdef __APPLE__
    // See getStride()
    if (getPF() == A8) {
        glPixelStorei(GL_UNPACK_ALIGNMENT, 1);
    } else {
        glPixelStorei(GL_UNPACK_ALIGNMENT, 4);
    }
#endif
    glTexSubImage2D(GL_TEXTURE_2D, 0, 0, 0, size.x, size.y,
            GLTexture::getGLFormat(getPF()), GLTexture::getGLType(getPF()), 0);
    GLContext::checkError("PBO::setImage: glTexSubImage2D()");
    glproc::BindBuffer(GL_PIXEL_UNPACK_BUFFER_EXT, 0);
    tex.setDirty();
    tex.generateMipmaps();
}

bool PBO::isReadPBO() const
{
    switch (m_Usage) {
        case GL_STATIC_DRAW:
        case GL_DYNAMIC_DRAW:
        case GL_STREAM_DRAW:
            return false;
        case GL_STATIC_READ:
        case GL_DYNAMIC_READ:
        case GL_STREAM_READ:
            return true;
        default:
            AVG_ASSERT(false);
            return false;
    }
}

unsigned PBO::getMemNeeded() const
{
    return getStride()*getSize().y;
}

unsigned PBO::getStride() const
{
    IntPoint size = getSize();
    unsigned stride = Bitmap::getPreferredStride(size.x, getPF());
#ifdef __APPLE__
    if (getPF() == A8) {
        // Workaround for apparent bug in Apple/NVidia drivers (Verified on OS X 10.6.8,
        // MBP early 2011): GL_UNPACK_ALIGNMENT != 1 causes broken A8 textures.
        stride = size.x;
    }
#endif
    return stride;
}

unsigned PBO::getTarget() const
{
    if (isReadPBO()) {
        return GL_PIXEL_PACK_BUFFER_EXT;
    } else {
        return GL_PIXEL_UNPACK_BUFFER_EXT;
    }
}

}
