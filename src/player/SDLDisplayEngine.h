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

#ifndef _SDLDisplayEngine_H_
#define _SDLDisplayEngine_H_

#include "../api.h"
#include "IEventSource.h"
#include "DisplayEngine.h"
#include "../graphics/Bitmap.h"
#include "../graphics/Pixel32.h"
#include "../graphics/OGLHelper.h"
#include "../graphics/OGLShader.h"
#include "../graphics/OGLProgram.h"
#include "../graphics/VertexArray.h"

#include <SDL/SDL.h>

#include <string>
#include <vector>

namespace avg {

class AVG_API SDLDisplayEngine: public DisplayEngine, public IEventSource
{
    public:
        SDLDisplayEngine();
        virtual ~SDLDisplayEngine();

        // From DisplayEngine
        virtual void init(const DisplayParams& DP);
        virtual void teardown();
        virtual double getRefreshRate();
        virtual void setGamma(double Red, double Green, double Blue);
        virtual void setMousePos(const IntPoint& pos);

        virtual void render(AVGNodePtr pRootNode);
        
        virtual bool pushClipRect(const DRect& rc);
        virtual void popClipRect();
        virtual const DRect& getClipRect();
        virtual void pushTransform(const DPoint& translate, double angle, 
                const DPoint& pivot);
        virtual void popTransform();

        virtual OGLTiledSurface * createTiledSurface();

        virtual int getWidth();
        virtual int getHeight();
        virtual int getBPP();

        virtual bool supportsBpp(int bpp);
        virtual bool hasRGBOrdering();
        
        virtual YCbCrMode getYCbCrMode();
        OGLShaderPtr getYCbCr420pShader();
        OGLShaderPtr getYCbCrJ420pShader();

        virtual OGLProgramPtr getActiveShader();
        virtual void setShaders(OGLShaderPtr pFragmentShader, OGLShaderPtr pVertexShader);
        
        virtual void showCursor(bool bShow);
        virtual BitmapPtr screenshot();

        // From IEventSource
        virtual std::vector<EventPtr> pollEvents();

        // Texture config.
        void initTextureMode();
        bool usePOTTextures();
        int getMaxTexSize();
        void enableTexture(bool bEnable);
        void enableGLColorArray(bool bEnable);
        
        int getOGLDestMode(PixelFormat pf);
        int getOGLSrcMode(PixelFormat pf);
        int getOGLPixelType(PixelFormat pf);
        OGLMemoryMode getMemoryModeSupported();

        void setOGLOptions(bool bUsePOW2Textures, YCbCrMode DesiredYcbcrMode, 
                bool bUsePixelBuffers, int MultiSampleSamples, 
                VSyncMode DesiredVSyncMode);
        
        long long getGPUMemoryUsage();
        void deregisterSurface(OGLTiledSurface *);
        unsigned createTexture(const IntPoint& size, PixelFormat pf);

        void pushShader();
        void popShader();
    private:
        void initSDL(int width, int height, bool isFullscreen, int bpp);
        void initInput();
        void initTranslationTable();
        void initJoysticks();
        void logConfig(); 
        virtual void swapBuffers();
        void clip(bool forward);
        void setClipPlane(double Eqn[4], int WhichPlane);
        void safeSetAttribute(SDL_GLattr attr, int value);
        static bool isParallels();

        EventPtr createMouseEvent
                (Event::Type Type, const SDL_Event & SDLEvent, long Button);
        EventPtr createMouseButtonEvent
                (Event::Type Type, const SDL_Event & SDLEvent);
        EventPtr createKeyEvent
                (Event::Type Type, const SDL_Event & SDLEvent);
        
        int m_Width;
        int m_Height;
        bool m_IsFullscreen;
        int m_bpp;
        int m_WindowWidth;
        int m_WindowHeight;
        std::vector<DRect> m_ClipRects;
        bool m_bEnableCrop;

        SDL_Surface * m_pScreen;
        
        OGLShaderPtr m_pCurrentFragmentShader;
        OGLShaderPtr m_pCurrentVertexShader;
        
        std::vector<OGLShaderPtr> m_ShaderStack;

        OGLProgramPtr m_pActiveShader;

        void checkYCbCrSupport();
        YCbCrMode m_YCbCrMode;
        OGLShaderPtr m_pYCbCrShader;
        OGLShaderPtr m_pYCbCrJShader;

        // Vertical blank stuff.
        virtual bool initVBlank(int rate);
        bool vbWait(int rate);
        enum VBMethod {VB_SGI, VB_APPLE, VB_DRI, VB_WIN, VB_NONE};
        VBMethod m_VBMethod;
        int m_VBMod;
        int m_LastVBCount;
        bool m_bFirstVBFrame;
        int m_dri_fd;

        static void calcRefreshRate();
        static double s_RefreshRate;

        bool m_bMouseOverApp;
        IntPoint m_LastMousePos;
        static std::vector<long> KeyCodeTranslationTable;

        // Texture config.
        bool m_bUsePOTTextures;
        int m_TextureMode;
        int m_MaxTexSize;
        bool m_bEnableTexture;
        bool m_bEnableGLColorArray;

        bool m_bShouldUsePOW2Textures;
        YCbCrMode m_DesiredYCbCrMode;
        bool m_bShouldUsePixelBuffers;
        int m_MultiSampleSamples;
        VSyncMode m_DesiredVSyncMode;

        bool m_bCheckedMemoryMode;
        OGLMemoryMode m_MemoryMode;
        
        std::vector<OGLTiledSurface *> m_pSurfaces;


};

}

#endif
