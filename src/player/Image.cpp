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

#include "Image.h"

#include "../base/Logger.h"
#include "../base/Exception.h"
#include "../base/ObjectCounter.h"

#include "../graphics/Filterfliprgb.h"

#include "SDLDisplayEngine.h"
#include "OGLSurface.h"

#include <Magick++.h>

#include <iostream>
#include <sstream>

using namespace std;

namespace avg {

Image::Image(OGLSurface * pSurface)
    : m_sFilename(""),
      m_pSurface(pSurface),
      m_pEngine(0),
      m_State(CPU),
      m_Source(NONE)
{
    ObjectCounter::get()->incRef(&typeid(*this));
    assertValid();
}

Image::~Image()
{
    if (m_State == GPU && m_Source != NONE) {
        m_pSurface->destroy();
    }
    ObjectCounter::get()->decRef(&typeid(*this));
}
        
void Image::moveToGPU(SDLDisplayEngine* pEngine)
{
    m_pEngine = pEngine;
    if (m_State == CPU) {
        switch (m_Source) {
            case FILE:
            case BITMAP:
                setupSurface();
                break;
            case SCENE:
                m_pSurface->create(m_pScene->getSize(), B8G8R8X8);
                m_pSurface->setTexID(m_pScene->getTexID());
                break;
            case NONE:
                break;
            default:
                AVG_ASSERT(false);
        }
        m_State = GPU;
    }
}

void Image::moveToCPU()
{
    if (m_State == GPU) {
        switch (m_Source) {
            case FILE:
            case BITMAP:
                m_pBmp = m_pSurface->readbackBmp();
                break;
            case SCENE:
                break;
            case NONE:
                break;
            default:
                AVG_ASSERT(false);
                return;
        }
        m_State = CPU;
        m_pEngine = 0;
        m_pSurface->destroy();
    }
}

void Image::discard()
{
    setEmpty();
    if (m_State == GPU) {
        m_pEngine = 0;
    }
    m_State = CPU;
}

void Image::setEmpty()
{
    if (m_State == GPU) {
        m_pSurface->destroy();
    }
    if (m_Source == FILE || m_Source == BITMAP) {
        m_sFilename = "";
        m_pBmp = BitmapPtr();
    }
    m_Source = NONE;
}

void Image::setFilename(const std::string& sFilename)
{
    AVG_TRACE(Logger::MEMORY, "Loading " << sFilename);
    BitmapPtr pBmp(new Bitmap(sFilename));
    changeSource(FILE);
    m_pBmp = pBmp;

    m_sFilename = sFilename;
    if (m_State == GPU) {
        m_pSurface->destroy();
        setupSurface();
    }
}

void Image::setBitmap(const Bitmap * pBmp)
{
    if (!pBmp) {
        throw Exception(AVG_ERR_UNSUPPORTED, "setBitmap(): bitmap must not be None!");
    }
    bool bSourceChanged = changeSource(BITMAP);
    PixelFormat pf = calcSurfacePF(*pBmp);
    if (m_State == GPU) {
        if (bSourceChanged || m_pSurface->getSize() != pBmp->getSize() ||
                m_pSurface->getPixelFormat() != pf)
        {
            m_pSurface->create(pBmp->getSize(), pf);
        }            
        BitmapPtr pSurfaceBmp = m_pSurface->lockBmp();
        pSurfaceBmp->copyPixels(*pBmp);
        m_pSurface->unlockBmps();
        m_pBmp = BitmapPtr();
    } else {
        m_pBmp = BitmapPtr(new Bitmap(pBmp->getSize(), pf, ""));
        m_pBmp->copyPixels(*pBmp);
    }
    m_sFilename = "";

}

void Image::setScene(OffscreenScenePtr pScene)
{
    if (m_Source == SCENE && pScene == m_pScene) {
        return;
    }
    changeSource(SCENE);
    m_pScene = pScene;
    if (m_State == GPU) {
        m_pSurface->create(m_pScene->getSize(), B8G8R8X8);
        m_pSurface->setTexID(m_pScene->getTexID());
    }
}

OffscreenScenePtr Image::getScene() const
{
    return m_pScene;
}

const string& Image::getFilename() const
{
    return m_sFilename;
}

BitmapPtr Image::getBitmap()
{
    if (m_Source == NONE) {
        return BitmapPtr();
    } else {
        switch (m_State) {
            case CPU:
                if (m_Source == SCENE) {
                    return BitmapPtr();
                } else {
                    return BitmapPtr(new Bitmap(*m_pBmp));
                }
            case GPU:
                return m_pSurface->readbackBmp();
            default:
                AVG_ASSERT(false);
                return BitmapPtr();
        }
    }
}

IntPoint Image::getSize()
{
    if (m_Source == NONE) {
        return IntPoint(0,0);
    } else {
        switch (m_State) {
            case CPU:
                if (m_Source == SCENE) {
                    return m_pScene->getSize();
                } else {
                    return m_pBmp->getSize();
                }
            case GPU:
                return m_pSurface->getSize();
            default:
                AVG_ASSERT(false);
                return IntPoint(0,0);
        }
    }
}

PixelFormat Image::getPixelFormat()
{
    if (m_Source == NONE) {
        return B8G8R8X8;
    } else {
        switch (m_State) {
            case CPU:
                if (m_Source == SCENE) {
                    return B8G8R8X8;
                } else {
                    return m_pBmp->getPixelFormat();
                }
            case GPU:
                return m_pSurface->getPixelFormat();
            default:
                AVG_ASSERT(false);
                return B8G8R8X8;
        }
    }
}

OGLSurface* Image::getSurface()
{
    AVG_ASSERT(m_State == GPU);
    return m_pSurface;
}

Image::State Image::getState()
{
    return m_State;
}

Image::Source Image::getSource()
{
    return m_Source;
}

SDLDisplayEngine* Image::getEngine()
{
    return m_pEngine;
}

void Image::setupSurface()
{
    PixelFormat pf = calcSurfacePF(*m_pBmp);
    m_pSurface->create(m_pBmp->getSize(), pf);
    BitmapPtr pSurfaceBmp = m_pSurface->lockBmp();
    pSurfaceBmp->copyPixels(*m_pBmp);
    m_pSurface->unlockBmps();
    m_pBmp=BitmapPtr();
}

PixelFormat Image::calcSurfacePF(const Bitmap& bmp)
{
    PixelFormat pf;
    pf = B8G8R8X8;
    if (bmp.hasAlpha()) {
        pf = B8G8R8A8;
    }
    if (bmp.getPixelFormat() == I8) {
        pf = I8;
    }
    return pf;
}

bool Image::changeSource(Source newSource)
{
    if (newSource != m_Source) {
        switch (m_Source) {
            case NONE:
                break;
            case FILE:
            case BITMAP:
                if (m_State == CPU) {
                    m_pBmp = BitmapPtr();
                }
                break;
            case SCENE:
                m_pScene = OffscreenScenePtr();
                break;
            default:
                AVG_ASSERT(false);
        }
        m_Source = newSource;
        return true;
    } else {
        return false;
    }
}

void Image::assertValid() const
{
    AVG_ASSERT(m_pSurface);
    AVG_ASSERT((m_Source == FILE || m_Source == BITMAP) ==
            (m_sFilename != ""));
    AVG_ASSERT((m_Source == SCENE) == bool(m_pScene));
    switch (m_State) {
        case CPU:
            AVG_ASSERT(!m_pEngine);
            AVG_ASSERT((m_Source == FILE || m_Source == BITMAP) ==
                    bool(m_pBmp));
//            AVG_ASSERT(!(m_pSurface->isCreated()));
            break;
        case GPU:
            AVG_ASSERT(m_pEngine);
            AVG_ASSERT(!m_pBmp);
/*
            if (m_Source != NONE) {
                AVG_ASSERT(m_pSurface->isCreated());
            } else {
                AVG_ASSERT(!m_pSurface->isCreated());
            }
*/            
            break;
        default:
            AVG_ASSERT(false);
    }
}

}
