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

#ifndef _VideoDecoder_H_
#define _VideoDecoder_H_

#include "../api.h"
#include "../avgconfigwrapper.h"

#include "VideoInfo.h"

#include "../audio/AudioParams.h"
#include "../graphics/PixelFormat.h"

#include <string>
#include <boost/shared_ptr.hpp>

struct vdpau_render_state;

namespace avg {

class Bitmap;
typedef boost::shared_ptr<Bitmap> BitmapPtr;

enum FrameAvailableCode {
    FA_NEW_FRAME, FA_USE_LAST_FRAME, FA_STILL_DECODING, FA_CLOSED
};

enum StreamSelect {
    SS_AUDIO, SS_VIDEO, SS_DEFAULT, SS_ALL
};

class AVG_API VideoDecoder
{
    public:
        enum DecoderState {CLOSED, OPENED, DECODING};
        virtual ~VideoDecoder() {};
        virtual void open(const std::string& sFilename, bool bUseHardwareAcceleration, 
                bool bEnableSound) = 0;
        virtual void startDecoding(bool bDeliverYCbCr, const AudioParams* pAP) = 0;
        virtual void close() = 0;
        virtual DecoderState getState() const = 0;
        virtual VideoInfo getVideoInfo() const = 0;

        virtual void seek(float destTime) = 0;
        virtual void loop() = 0;
        virtual IntPoint getSize() const = 0;
        virtual int getCurFrame() const = 0;
        virtual int getNumFramesQueued() const = 0;
        virtual float getCurTime(StreamSelect stream = SS_DEFAULT) const = 0;
        virtual float getNominalFPS() const = 0;
        virtual float getFPS() const = 0;
        virtual void setFPS(float fps) = 0;
        virtual PixelFormat getPixelFormat() const = 0;

        virtual FrameAvailableCode renderToBmp(BitmapPtr pBmp,
                float timeWanted);
        virtual FrameAvailableCode renderToBmps(std::vector<BitmapPtr>& pBmps,
                float timeWanted) = 0;
        virtual FrameAvailableCode renderToVDPAU(vdpau_render_state** ppRenderState);
        virtual bool isEOF(StreamSelect stream = SS_ALL) const = 0;
        virtual void throwAwayFrame(float timeWanted) = 0;

        static void logConfig();
};

typedef boost::shared_ptr<VideoDecoder> VideoDecoderPtr;

void avcodecError(const std::string& sFilename, int err);

void copyPlaneToBmp(BitmapPtr pBmp, unsigned char * pData, int stride);

}
#endif 

