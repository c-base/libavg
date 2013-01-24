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

#include "SyncDecoder.h"
#include "AsyncDemuxer.h"
#include "FFMpegDemuxer.h"

#include "../base/Exception.h"
#include "../base/Logger.h"
#include "../base/ScopeTimer.h"
#include "../base/ObjectCounter.h"
#include "../base/ProfilingZoneID.h"
#include "../base/StringHelper.h"

#include "../audio/AudioBuffer.h"

#include "../graphics/BitmapLoader.h"

#include <iostream>
#include <sstream>
#ifndef _WIN32
#include <unistd.h>
#endif

using namespace std;
using namespace boost;

namespace avg {

SyncDecoder::SyncDecoder()
    : m_pSwsContext(0),
      m_bUseStreamFPS(true),
      m_pDemuxer(0),
      m_bFirstPacket(false),
      m_VideoStartTimestamp(-1),
      m_LastVideoFrameTime(-1),
      m_FPS(0)
{
    ObjectCounter::get()->incRef(&typeid(*this));
}

SyncDecoder::~SyncDecoder()
{
    ObjectCounter::get()->decRef(&typeid(*this));
}

void SyncDecoder::open(const string& sFilename, bool bUseHardwareAcceleration, 
        bool bEnableSound)
{
    m_bVideoEOF = false;
    m_bEOFPending = false;
    m_VideoStartTimestamp = -1;
    VideoDecoder::open(sFilename, false, false);

    if (getVStreamIndex() >= 0) {
        
        // Set video parameters
        m_TimeUnitsPerSecond = float(1.0/av_q2d(getVideoStream()->time_base));
        if (m_bUseStreamFPS) {
            m_FPS = getStreamFPS();
        }
        m_bFirstPacket = true;
        m_LastVideoFrameTime = -1;
        m_bVideoSeekDone = false;
    }
}

void SyncDecoder::startDecoding(bool bDeliverYCbCr, const AudioParams* pAP)
{
    VideoDecoder::startDecoding(bDeliverYCbCr, 0);

    AVG_ASSERT(!m_pDemuxer);
    vector<int> streamIndexes;
    streamIndexes.push_back(getVStreamIndex());
    m_pDemuxer = new FFMpegDemuxer(getFormatContext(), streamIndexes);
}

void SyncDecoder::close() 
{
    delete m_pDemuxer;
    m_pDemuxer = 0;
    
    if (m_pSwsContext) {
        sws_freeContext(m_pSwsContext);
        m_pSwsContext = 0;
    }
    VideoDecoder::close();
}

void SyncDecoder::seek(float destTime) 
{
    AVG_ASSERT(getState() == DECODING);

    if (m_bFirstPacket) {
        AVFrame frame;
        readFrame(frame);
    }
    dynamic_cast<FFMpegDemuxer*>(m_pDemuxer)
            ->seek(destTime + m_VideoStartTimestamp/m_TimeUnitsPerSecond);
    m_bVideoSeekDone = true;
    m_bVideoEOF = false;
    m_LastVideoFrameTime = -1.0f;
    avcodec_flush_buffers(getCodecContext());
}

void SyncDecoder::loop()
{
    seek(0);
}

int SyncDecoder::getCurFrame() const
{
    AVG_ASSERT(getState() != CLOSED);
    return int(m_LastVideoFrameTime*getStreamFPS()+0.5);
}

int SyncDecoder::getNumFramesQueued() const
{
    return 0;
}

float SyncDecoder::getCurTime(StreamSelect stream) const
{
    AVG_ASSERT(getState() != CLOSED);
    AVG_ASSERT(stream != SS_AUDIO);
    return m_LastVideoFrameTime;
}

float SyncDecoder::getFPS() const
{
    AVG_ASSERT(getState() != CLOSED);
    return m_FPS;
}

void SyncDecoder::setFPS(float fps)
{
    m_bUseStreamFPS = (fps == 0);
    if (fps == 0) {
        m_FPS = getStreamFPS();
    } else {
        m_FPS = fps;
    }
}

static ProfilingZoneID RenderToBmpProfilingZone("FFMpeg: renderToBmp", true);
static ProfilingZoneID CopyImageProfilingZone("FFMpeg: copy image", true);
static ProfilingZoneID VDPAUCopyProfilingZone("FFMpeg: VDPAU copy", true);

FrameAvailableCode SyncDecoder::renderToBmps(vector<BitmapPtr>& pBmps, float timeWanted)
{
    AVG_ASSERT(getState() == DECODING);
    ScopeTimer timer(RenderToBmpProfilingZone);
    AVFrame frame;
    FrameAvailableCode frameAvailable;
    if (timeWanted == -1) {
        readFrame(frame);
        frameAvailable = FA_NEW_FRAME;
    } else {
        frameAvailable = readFrameForTime(frame, timeWanted);
    }
    if (!m_bVideoEOF && frameAvailable == FA_NEW_FRAME) {
        if (pixelFormatIsPlanar(getPixelFormat())) {
            ScopeTimer timer(CopyImageProfilingZone);
            for (unsigned i = 0; i < pBmps.size(); ++i) {
                copyPlaneToBmp(pBmps[i], frame.data[i], frame.linesize[i]);
            }
        } else {
            convertFrameToBmp(frame, pBmps[0]);
        }
        return FA_NEW_FRAME;
    }
    // TODO: clean this up.
    return FA_USE_LAST_FRAME;
}

void SyncDecoder::throwAwayFrame(float timeWanted)
{
    AVG_ASSERT(getState() == DECODING);
    AVFrame frame;
    readFrameForTime(frame, timeWanted);
}

bool SyncDecoder::isEOF(StreamSelect stream) const
{
    AVG_ASSERT(getState() == DECODING);
    AVG_ASSERT(stream != SS_AUDIO);
    return m_bVideoEOF;
}

static ProfilingZoneID ConvertImageLibavgProfilingZone(
        "FFMpeg: colorspace conv (libavg)", true);
static ProfilingZoneID ConvertImageSWSProfilingZone(
        "FFMpeg: colorspace conv (SWS)", true);
static ProfilingZoneID SetAlphaProfilingZone("FFMpeg: set alpha channel", true);

void SyncDecoder::convertFrameToBmp(AVFrame& frame, BitmapPtr pBmp)
{
    AVPicture destPict;
    unsigned char * pDestBits = pBmp->getPixels();
    destPict.data[0] = pDestBits;
    destPict.linesize[0] = pBmp->getStride();
    AVPixelFormat destFmt;
    switch (pBmp->getPixelFormat()) {
        case R8G8B8X8:
        case R8G8B8A8:
            destFmt = PIX_FMT_RGBA;
            break;
        case B8G8R8X8:
        case B8G8R8A8:
            destFmt = PIX_FMT_BGRA;
            break;
        case R8G8B8:
            destFmt = PIX_FMT_RGB24;
            break;
        case B8G8R8:
            destFmt = PIX_FMT_BGR24;
            break;
        case YCbCr422:
            destFmt = PIX_FMT_YUYV422;
            break;
        default:
            AVG_ASSERT_MSG(false, (string("SyncDecoder: Dest format ") +
                    toString(pBmp->getPixelFormat()) + " not supported.").c_str());
            destFmt = PIX_FMT_BGRA;
    }
    AVCodecContext const* pContext = getCodecContext();
    if (destFmt == PIX_FMT_BGRA && (pContext->pix_fmt == PIX_FMT_YUV420P || 
                pContext->pix_fmt == PIX_FMT_YUVJ420P))
    {
        ScopeTimer timer(ConvertImageLibavgProfilingZone);
        BitmapPtr pBmpY(new Bitmap(pBmp->getSize(), I8, frame.data[0],
                    frame.linesize[0], false));
        BitmapPtr pBmpU(new Bitmap(pBmp->getSize(), I8, frame.data[1],
                    frame.linesize[1], false));
        BitmapPtr pBmpV(new Bitmap(pBmp->getSize(), I8, frame.data[2],
                    frame.linesize[2], false));
        pBmp->copyYUVPixels(*pBmpY, *pBmpU, *pBmpV, 
                pContext->pix_fmt == PIX_FMT_YUVJ420P);
    } else {
        if (!m_pSwsContext) {
            m_pSwsContext = sws_getContext(pContext->width, pContext->height, 
                    pContext->pix_fmt, pContext->width, pContext->height, destFmt, 
                    SWS_BICUBIC, 0, 0, 0);
            AVG_ASSERT(m_pSwsContext);
        }
        {
            ScopeTimer timer(ConvertImageSWSProfilingZone);
            sws_scale(m_pSwsContext, frame.data, frame.linesize, 0, 
                    pContext->height, destPict.data, destPict.linesize);
        }
        if (pBmp->getPixelFormat() == B8G8R8X8 || pBmp->getPixelFormat() == R8G8B8X8) {
            ScopeTimer timer(SetAlphaProfilingZone);
            // Make sure the alpha channel is white.
            // TODO: This is slow. Make OpenGL do it.
            unsigned char * pLine = pBmp->getPixels();
            IntPoint size = pBmp->getSize();
            for (int y = 0; y < size.y; ++y) {
                unsigned char * pPixel = pLine;
                for (int x = 0; x < size.x; ++x) {
                    pPixel[3] = 0xFF;
                    pPixel += 4;
                }
                pLine = pLine + pBmp->getStride();
            }
        }
    }
}
       
FrameAvailableCode SyncDecoder::readFrameForTime(AVFrame& frame, float timeWanted)
{
    AVG_ASSERT(getState() == DECODING);
    AVG_ASSERT(timeWanted != -1);
    float timePerFrame = 1.0f/m_FPS;
    if (!m_bVideoSeekDone && timeWanted-m_LastVideoFrameTime < 0.5f*timePerFrame) {
        // The last frame is still current. Display it again.
        return FA_USE_LAST_FRAME;
    } else {
        bool bInvalidFrame = true;
        while (bInvalidFrame && !m_bVideoEOF) {
            float frameTime = readFrame(frame);
            bInvalidFrame = frameTime-timeWanted < -0.5f*timePerFrame;
        }
    }
    if (m_bVideoSeekDone) {
        m_bVideoSeekDone = false;
    }
    return FA_NEW_FRAME;
}

static ProfilingZoneID DecodeProfilingZone("FFMpeg: decode", true);

float SyncDecoder::readFrame(AVFrame& frame)
{
    AVG_ASSERT(getState() == DECODING);
    ScopeTimer timer(DecodeProfilingZone); 

    if (m_bEOFPending) {
        m_bVideoEOF = true;
        m_bEOFPending = false;
        return m_LastVideoFrameTime;
    }
    AVCodecContext* pContext = getCodecContext();
    int bGotPicture = 0;
    AVPacket* pPacket = 0;
    float frameTime = -32768;
    bool bDone = false;
    while (!bGotPicture && !bDone) {
        pPacket = m_pDemuxer->getPacket(getVStreamIndex());
        m_bFirstPacket = false;
        if (pPacket) {
#if LIBAVCODEC_VERSION_INT > AV_VERSION_INT(52, 31, 0)
            int len1 = avcodec_decode_video2(pContext, &frame, &bGotPicture, pPacket);
#else
            int len1 = avcodec_decode_video(pContext, &frame, &bGotPicture, pPacket->data,
                    pPacket->size);
#endif
            if (len1 > 0) {
                AVG_ASSERT(len1 == pPacket->size);
            }
            if (bGotPicture) {
                frameTime = getFrameTime(pPacket->dts);
            }
            av_free_packet(pPacket);
            delete pPacket;
        } else {
            // No more packets -> EOF. Decode the last data we got.
#if LIBAVCODEC_VERSION_INT > AV_VERSION_INT(52, 31, 0)
            AVPacket packet;
            packet.data = 0;
            packet.size = 0;
            avcodec_decode_video2(pContext, &frame, &bGotPicture, &packet);
#else
            avcodec_decode_video(pContext, &frame, &bGotPicture, 0, 0);
#endif
            if (bGotPicture) {
                m_bEOFPending = true;
            } else {
                m_bVideoEOF = true;
            }
            // We don't have a timestamp for the last frame, so we'll
            // calculate it based on the frame before.
            frameTime = m_LastVideoFrameTime+1.0f/m_FPS;
            m_LastVideoFrameTime = frameTime;
        }
        if (m_bVideoEOF) {
            bDone = true;
        }
    }
    AVG_ASSERT(frameTime != -32768);
    return frameTime;
/*
    cerr << "coded_picture_number: " << frame.coded_picture_number <<
            ", display_picture_number: " << frame.display_picture_number <<
            ", pts: " << frame.pts << endl;

    cerr << "key_frame: " << frame.key_frame << 
           ", pict_type: " << frame.pict_type << endl;
    AVFrac spts = m_pVStream->pts;
    cerr << "Stream.pts: " << spts.val + float(spts.num)/spts.den << endl;
*/
}

float SyncDecoder::getFrameTime(long long dts)
{
    if (dts == (long long)AV_NOPTS_VALUE) {
        dts = 0;
    }
    if (m_VideoStartTimestamp == -1) {
        m_VideoStartTimestamp = dts;
    }
    float frameTime;
    if (m_bUseStreamFPS || m_bVideoSeekDone) {
        frameTime = float(dts-m_VideoStartTimestamp)/m_TimeUnitsPerSecond;
    } else {
        if (m_LastVideoFrameTime == -1) {
            frameTime = 0;
        } else {
            frameTime = m_LastVideoFrameTime + 1.0f/m_FPS;
        }
    }
    m_LastVideoFrameTime = frameTime;
    return frameTime;
}

}

