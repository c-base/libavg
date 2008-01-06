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

#ifndef _Video_H_
#define _Video_H_

// Python docs say python.h should be included before any standard headers (!)
#include "WrapPython.h" 

#include "Node.h"
#include "VideoBase.h"
#include "AudioSource.h"

#include "../base/Point.h"
#include "../base/IFrameListener.h"

#include <string>

namespace avg {

class IVideoDecoder;

class Video : public VideoBase, IFrameListener, public AudioSource
{
    public:
        Video (const xmlNodePtr xmlNode, Player * pPlayer);
        virtual ~Video ();
        
        virtual void setRenderingEngines(DisplayEngine * pDisplayEngine, AudioEngine * pAudioEngine);
        virtual void disconnect();

        const std::string& getHRef() const;
        void setHRef(const std::string& href);
        double getSpeedFactor() const;
        void setSpeedFactor(double SpeedFactor);
        void setVolume(double Volume);
        void checkReload();

        int getNumFrames() const;
        int getCurFrame() const;
        void seekToFrame(int FrameNum);
        bool getLoop() const;
        bool isThreaded() const;
        void setEOFCallback(PyObject * pEOFCallback);

        virtual std::string getTypeStr();

        virtual void onFrameEnd();
        
        virtual void setAudioEnabled(bool bEnabled);
        virtual void fillAudioFrame(AudioFrame* frame);

    protected:
        virtual void changeVideoState(VideoState NewVideoState);

    private:
        void initVideoSupport();

        bool renderToSurface(ISurface * pSurface);
        bool canRenderToBackbuffer(int BitsPerPixel);
        void seek(int DestFrame);
        void onEOF();
       
        virtual void open(YCbCrMode ycbcrMode);
        virtual void close();
        virtual PixelFormat getPixelFormat();
        virtual IntPoint getMediaSize();
        virtual double getFPS();
        virtual long long getCurTime();

        std::string m_href;
        std::string m_Filename;
        bool m_bLoop;
        bool m_bThreaded;
        double m_FPS;
        double m_SpeedFactor;
        bool m_bEOFPending;
        PyObject * m_pEOFCallback;
        int m_FramesTooLate;
        int m_FramesPlayed;
        bool m_bAudioEnabled;

        int m_CurFrame;
        long long m_StartTime;
        long long m_PauseTime;
        long long m_PauseStartTime;

        IVideoDecoder * m_pDecoder;

        static bool m_bInitialized;
};

}

#endif 

