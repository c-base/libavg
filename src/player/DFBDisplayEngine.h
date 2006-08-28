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

#ifndef _DFBDisplayEngine_H_
#define _DFBDisplayEngine_H_

#include "IEventSource.h"
#include "DisplayEngine.h"

#include <directfb/directfb.h>

#include <string>

namespace avg {

class DFBDisplayEngine: public DisplayEngine, public IEventSource
{
    public:
        DFBDisplayEngine();
        virtual ~DFBDisplayEngine();

        // From IDisplayEngine
        virtual void init(const DisplayParams& DP);
        virtual void teardown();
        virtual double getRefreshRate();
        virtual void setGamma(double Red, double Green, double Blue);

        virtual void render(AVGNodePtr pRootNode, bool bRenderEverything);
        
        virtual void setClipRect();
        virtual bool pushClipRect(const DRect& rc, bool bClip);
        virtual void popClipRect();
        virtual const DRect& getClipRect();
        virtual void blt32(ISurface * pSurface, const DRect* pDestRect, 
                double opacity, double angle, const DPoint& pivot,
                BlendMode Mode);
        virtual void blta8(ISurface * pSurface, const DRect* pDestRect, 
                double opacity, const Pixel32& color, double angle,
                const DPoint& pivot, BlendMode Mode);

        virtual ISurface * createSurface();

        IDirectFBSurface * getPrimary();
        void DFBErrorCheck(int avgcode, std::string where, DFBResult dfbcode); 

        virtual int getWidth();
        virtual int getHeight();
        virtual int getBPP();

        virtual bool supportsBpp(int bpp);
        virtual bool hasRGBOrdering();
        virtual DisplayEngine::YCbCrMode getYCbCrMode(); 

        virtual void showCursor (bool bShow);
        virtual BitmapPtr screenshot ();

        // From IEventSource
        virtual std::vector<Event *> pollEvents();
       
    private:
        void initDFB(int width, int height, bool isFullscreen, int bpp);
        void initLayer(int width, int height);
        void initInput();
        void initBackbuffer();
        void clear();
        void setDirtyRect(const DRect& rc);
        virtual void swapBuffers(const Region & UpdateRegion);

        void blt32(IDirectFBSurface * pSrc, const DRect* pDestRect, 
                double opacity, bool bAlpha, BlendMode Mode);
        void blt(IDirectFBSurface * pSrc, const DRect* pDestRect);

        void dumpSurface(IDirectFBSurface * pSurf, const std::string & name);

        Event * createEvent(const char * pTypeName);
        int translateModifiers(DFBInputDeviceModifierMask DFBModifiers);
        Event * createEvent(DFBWindowEvent* pdfbwEvent);
      
        virtual bool initVBlank(int rate);
        virtual bool vbWait(int rate);

        int m_Width;
        int m_Height;
        bool m_IsFullscreen;
        int m_bpp;
        DRect m_ClipRect;
        DRect m_DirtyRect;

        IDirectFB * m_pDirectFB;
        IDirectFBWindow * m_pDFBWindow;
        IDirectFBDisplayLayer * m_pDFBLayer;
        IDirectFBSurface * m_pBackBuffer;
        
        IDirectFBEventBuffer * m_pEventBuffer;
};

}

#endif //_DFBDisplayEngine_H_
