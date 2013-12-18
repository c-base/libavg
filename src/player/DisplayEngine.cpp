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

#include "DisplayEngine.h"
#include "../avgconfigwrapper.h"

#ifdef __APPLE__
#include "SDLMain.h"
#endif

#include "Event.h"
#include "MouseEvent.h"
#include "KeyEvent.h"
#include "Window.h"

#include "../base/Exception.h"
#include "../base/Logger.h"
#include "../base/ScopeTimer.h"
#include "../base/TimeSource.h"

#include "../graphics/Display.h"

#include "../video/VideoDecoder.h"

#ifdef __APPLE__
#include <ApplicationServices/ApplicationServices.h>
#endif

#include <SDL/SDL.h>

#ifdef __APPLE__
#include <OpenGL/OpenGL.h>
#endif

#ifdef AVG_ENABLE_XINERAMA
#include <X11/extensions/Xinerama.h>
#endif

#include <signal.h>
#include <iostream>

#ifdef _WIN32
#include <windows.h>
#endif

using namespace std;

namespace avg {

void DisplayEngine::initSDL()
{
#ifdef __APPLE__
    static bool bSDLInitialized = false;
    if (!bSDLInitialized) {
        CustomSDLMain();
        bSDLInitialized = true;
    }
#endif
#ifdef linux
    // Disable all other video drivers (DirectFB, libcaca, ...) to avoid confusing
    // error messages.
    SDL_putenv((char*)"SDL_VIDEODRIVER=x11");
#endif
    int err = SDL_InitSubSystem(SDL_INIT_VIDEO);
    if (err == -1) {
        throw Exception(AVG_ERR_VIDEO_INIT_FAILED, SDL_GetError());
    }
}

void DisplayEngine::quitSDL()
{
#ifndef _WIN32
    SDL_QuitSubSystem(SDL_INIT_VIDEO);
#endif
}

DisplayEngine::DisplayEngine()
    : IInputDevice("DisplayEngine"),
      m_Size(0,0),
      m_NumFrames(0),
      m_VBRate(0),
      m_Framerate(60),
      m_bInitialized(false),
      m_EffFramerate(0)
{
    initSDL();

    m_Gamma[0] = 1.0;
    m_Gamma[1] = 1.0;
    m_Gamma[2] = 1.0;
}

DisplayEngine::~DisplayEngine()
{
}

void DisplayEngine::init(const DisplayParams& dp, GLConfig glConfig) 
{
    if (m_Gamma[0] != 1.0f || m_Gamma[1] != 1.0f || m_Gamma[2] != 1.0f) {
        internalSetGamma(1.0f, 1.0f, 1.0f);
    }

    m_pWindow = WindowPtr(new Window(dp, glConfig));
    Display::get()->getRefreshRate();

    setGamma(dp.m_Gamma[0], dp.m_Gamma[1], dp.m_Gamma[2]);
    showCursor(dp.m_bShowCursor);
    if (dp.m_Framerate == 0) {
        setVBlankRate(dp.m_VBRate);
    } else {
        setFramerate(dp.m_Framerate);
    }

    // SDL sets up a signal handler we really don't want.
    signal(SIGSEGV, SIG_DFL);
    VideoDecoder::logConfig();

    SDL_EnableUNICODE(1);
}

void DisplayEngine::teardown()
{
    m_pWindow = WindowPtr();
}

void DisplayEngine::initRender()
{
    m_NumFrames = 0;
    m_FramesTooLate = 0;
    m_TimeSpentWaiting = 0;
    m_StartTime = TimeSource::get()->getCurrentMicrosecs();
    m_LastFrameTime = m_StartTime;
    m_bInitialized = true;
    if (m_VBRate != 0) {
        setVBlankRate(m_VBRate);
    } else {
        setFramerate(m_Framerate);
    }
}

void DisplayEngine::deinitRender()
{
    AVG_TRACE(Logger::category::PROFILE, Logger::severity::INFO,
            "Framerate statistics: ");
    AVG_TRACE(Logger::category::PROFILE, Logger::severity::INFO,
            "  Total frames: " <<m_NumFrames);
    float TotalTime = float(TimeSource::get()->getCurrentMicrosecs()
            -m_StartTime)/1000000;
    AVG_TRACE(Logger::category::PROFILE,  Logger::severity::INFO,
            "  Total time: " << TotalTime << " seconds");
    float actualFramerate = (m_NumFrames+1)/TotalTime;
    AVG_TRACE(Logger::category::PROFILE,  Logger::severity::INFO,
            "  Framerate achieved: " << actualFramerate);
    AVG_TRACE(Logger::category::PROFILE,  Logger::severity::INFO,
            "  Frames too late: " << m_FramesTooLate);
    AVG_TRACE(Logger::category::PROFILE,  Logger::severity::INFO,
            "  Percent of time spent waiting: " 
            << float (m_TimeSpentWaiting)/(10000*TotalTime));
    if (m_Framerate != 0) {
        AVG_TRACE(Logger::category::PROFILE,  Logger::severity::INFO,
                "  Framerate goal was: " << m_Framerate);
        if (m_Framerate*2 < actualFramerate && m_NumFrames > 10) {
            AVG_LOG_WARNING("Actual framerate was a lot higher than framerate goal.\
                    Is vblank sync forced off?");
        }
    }
    m_bInitialized = false;
}

void DisplayEngine::setFramerate(float rate)
{
    if (rate != 0 && m_bInitialized) {
        GLContext::getCurrent()->initVBlank(0);
    }
    m_Framerate = rate;
    m_VBRate = 0;
}

float DisplayEngine::getFramerate()
{
    return m_Framerate;
}

float DisplayEngine::getEffectiveFramerate()
{
    return m_EffFramerate;
}

void DisplayEngine::setVBlankRate(int rate)
{
    m_VBRate = rate;
    if (m_bInitialized) {
        bool bOK = GLContext::getCurrent()->initVBlank(rate);
        m_Framerate = Display::get()->getRefreshRate()/m_VBRate;
        if (!bOK || rate == 0) { 
            AVG_LOG_WARNING("Using framerate of " << m_Framerate << 
                    " instead of VBRate of " << m_VBRate);
            m_VBRate = 0;
        }
    }
}

bool DisplayEngine::wasFrameLate()
{
    return m_bFrameLate;
}

void DisplayEngine::setGamma(float red, float green, float blue)
{
    if (red > 0) {
        bool bOk = internalSetGamma(red, green, blue);
        m_Gamma[0] = red;
        m_Gamma[1] = green;
        m_Gamma[2] = blue;
        if (!bOk) {
            AVG_LOG_WARNING("Unable to set display gamma.");
        }
    }
}

void DisplayEngine::setMousePos(const IntPoint& pos)
{
    SDL_WarpMouse(pos.x, pos.y);
}

int DisplayEngine::getKeyModifierState() const
{
    return SDL_GetModState();
}

IntPoint DisplayEngine::calcWindowSize(const DisplayParams& dp) const
{
    float aspectRatio = float(dp.m_Size.x)/float(dp.m_Size.y);
    IntPoint windowSize;
    if (dp.m_WindowSize == IntPoint(0, 0)) {
        windowSize = dp.m_Size;
    } else if (dp.m_WindowSize.x == 0) {
        windowSize.x = int(dp.m_WindowSize.y*aspectRatio);
        windowSize.y = dp.m_WindowSize.y;
    } else {
        windowSize.x = dp.m_WindowSize.x;
        windowSize.y = int(dp.m_WindowSize.x/aspectRatio);
    }
    AVG_ASSERT(windowSize.x != 0 && windowSize.y != 0);
    return windowSize;
}
 
void DisplayEngine::setWindowTitle(const string& sTitle)
{
    SDL_WM_SetCaption(sTitle.c_str(), 0);
}

static ProfilingZoneID WaitProfilingZone("Render - wait");

void DisplayEngine::frameWait()
{
    ScopeTimer Timer(WaitProfilingZone);

    m_NumFrames++;

    m_FrameWaitStartTime = TimeSource::get()->getCurrentMicrosecs();
    m_TargetTime = m_LastFrameTime+(long long)(1000000/m_Framerate);
    m_bFrameLate = false;
    if (m_VBRate == 0) {
        if (m_FrameWaitStartTime <= m_TargetTime) {
            long long WaitTime = (m_TargetTime-m_FrameWaitStartTime)/1000;
            if (WaitTime > 5000) {
                AVG_LOG_WARNING("DisplayEngine: waiting " << WaitTime << " ms.");
            }
            TimeSource::get()->sleepUntil(m_TargetTime/1000);
        }
    }
}

void DisplayEngine::swapBuffers()
{
    m_pWindow->swapBuffers();
}

void DisplayEngine::checkJitter()
{
    if (m_LastFrameTime == 0) {
        m_EffFramerate = 0;
    } else {
        long long CurIntervalTime = TimeSource::get()->getCurrentMicrosecs()
                -m_LastFrameTime;
        m_EffFramerate = 1000000.0f/CurIntervalTime;
    }

    long long frameTime = TimeSource::get()->getCurrentMicrosecs();
    int maxDelay;
    if (m_VBRate == 0) {
        maxDelay = 2;
    } else {
        maxDelay = 6;
    }
    if ((frameTime - m_TargetTime)/1000 > maxDelay || m_bFrameLate) {
        m_bFrameLate = true;
        m_FramesTooLate++;
    }

    m_LastFrameTime = frameTime;
    m_TimeSpentWaiting += m_LastFrameTime-m_FrameWaitStartTime;
//    cerr << m_LastFrameTime << ", m_FrameWaitStartTime=" << m_FrameWaitStartTime << endl;
//    cerr << m_TimeSpentWaiting << endl;
}
   
long long DisplayEngine::getDisplayTime() 
{
    return (m_LastFrameTime-m_StartTime)/1000;
}

const IntPoint& DisplayEngine::getSize() const
{
    return m_Size;
}

IntPoint DisplayEngine::getWindowSize() const
{
    if (m_pWindow) {
        return m_pWindow->getSize();
    } else {
        return IntPoint(0,0);
    }
}

bool DisplayEngine::isFullscreen() const
{
    return m_bIsFullscreen;
}

void DisplayEngine::showCursor(bool bShow)
{
#ifdef _WIN32
#define MAX_CORE_POINTERS   6
    // Hack to fix a pointer issue with fullscreen, SDL and touchscreens
    // Refer to Mantis bug #140
    for (int i = 0; i < MAX_CORE_POINTERS; ++i) {
        ShowCursor(bShow);
    }
#else
    if (bShow) {
        SDL_ShowCursor(SDL_ENABLE);
    } else {
        SDL_ShowCursor(SDL_DISABLE);
    }
#endif
}

BitmapPtr DisplayEngine::screenshot(int buffer)
{
    return m_pWindow->screenshot(buffer);
}

vector<EventPtr> DisplayEngine::pollEvents()
{
    return m_pWindow->pollEvents();
}

bool DisplayEngine::internalSetGamma(float red, float green, float blue)
{
#ifdef __APPLE__
    // Workaround for broken SDL_SetGamma for libSDL 1.2.15 under Lion
    CGError err = CGSetDisplayTransferByFormula(kCGDirectMainDisplay, 0, 1, 1/red,
            0, 1, 1/green, 0, 1, 1/blue);
    return (err == CGDisplayNoErr);
#else
    int err = SDL_SetGamma(float(red), float(green), float(blue));
    return (err != -1);
#endif
}

}
