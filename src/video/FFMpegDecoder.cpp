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

#include "FFMpegDecoder.h"
#include "AsyncDemuxer.h"
#include "FFMpegDemuxer.h"
#ifdef AVG_ENABLE_VDPAU
#include "VDPAUDecoder.h"
#endif

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

#define SAMPLE_BUFFER_SIZE ((AVCODEC_MAX_AUDIO_FRAME_SIZE*3))

namespace avg {

bool FFMpegDecoder::s_bInitialized = false;
mutex FFMpegDecoder::s_OpenMutex;

FFMpegDecoder::FFMpegDecoder()
    : m_State(CLOSED),
      m_pFormatContext(0),
      m_PF(NO_PIXELFORMAT),
      m_pSwsContext(0),
      m_Size(0,0),
      m_bUseStreamFPS(true),
      m_AStreamIndex(-1),
      m_pAudioResampleContext(0),
      m_Volume(1.0),
      m_LastVolume(1.0),
      m_pDemuxer(0),
      m_pVStream(0),
      m_pAStream(0),
#ifdef AVG_ENABLE_VDPAU
      m_pVDPAUDecoder(0),
#endif
      m_VStreamIndex(-1),
      m_bFirstPacket(false),
      m_VideoStartTimestamp(-1),
      m_LastVideoFrameTime(-1),
      m_FPS(0)
{
    ObjectCounter::get()->incRef(&typeid(*this));
    initVideoSupport();
}

FFMpegDecoder::~FFMpegDecoder()
{
    if (m_pFormatContext) {
        close();
    }
#ifdef AVG_ENABLE_VDPAU
    if (m_pVDPAUDecoder) {
        delete m_pVDPAUDecoder;
    }
#endif
    ObjectCounter::get()->decRef(&typeid(*this));
}

void avcodecError(const string& sFilename, int err)
{
#if LIBAVFORMAT_VERSION_MAJOR > 52
        char buf[256];
        av_strerror(err, buf, 256);
        throw Exception(AVG_ERR_VIDEO_INIT_FAILED, sFilename + ": " + buf);
#else
    switch(err) {
        case AVERROR_NUMEXPECTED:
            throw Exception(AVG_ERR_VIDEO_INIT_FAILED, 
                    sFilename + ": Incorrect image filename syntax (use %%d to specify the image number:");
        case AVERROR_INVALIDDATA:
            throw Exception(AVG_ERR_VIDEO_INIT_FAILED, 
                    sFilename + ": Error while parsing header");
        case AVERROR_NOFMT:
            throw Exception(AVG_ERR_VIDEO_INIT_FAILED, 
                    sFilename + ": Unknown format");
        default:
            stringstream s;
            s << "'" << sFilename <<  "': Error while opening file (Num:" << err << ")";
            throw Exception(AVG_ERR_VIDEO_INIT_FAILED, s.str());
    }
#endif
}

int FFMpegDecoder::openCodec(int streamIndex, bool bUseHardwareAcceleration)
{
    AVCodecContext* pContext;
    pContext = m_pFormatContext->streams[streamIndex]->codec;
//    pContext->debug = 0x0001; // see avcodec.h

    AVCodec * pCodec = 0;
#ifdef AVG_ENABLE_VDPAU
    if (bUseHardwareAcceleration) {
        m_pVDPAUDecoder = new VDPAUDecoder();
        pContext->opaque = m_pVDPAUDecoder;
        pCodec = m_pVDPAUDecoder->openCodec(pContext);
    } 
#endif
    if (!pCodec) {
        pCodec = avcodec_find_decoder(pContext->codec_id);
    }
    if (!pCodec) {
        return -1;
    }
#if LIBAVCODEC_VERSION_INT > AV_VERSION_INT(53, 8, 0)
    int rc = avcodec_open2(pContext, pCodec, 0);
#else
    int rc = avcodec_open(pContext, pCodec);
#endif

    if (rc < 0) {
        return -1;
    }
    return 0;
}

void FFMpegDecoder::open(const string& sFilename, bool bThreadedDemuxer,
        bool bUseHardwareAcceleration, bool bEnableSound)
{
    mutex::scoped_lock lock(s_OpenMutex);
    m_bThreadedDemuxer = bThreadedDemuxer;
    m_bAudioEOF = false;
    m_bVideoEOF = false;
    m_bEOFPending = false;
    m_VideoStartTimestamp = -1;
    int err;
    m_sFilename = sFilename;

    AVG_TRACE(Logger::MEMORY, "Opening " << sFilename);
#if LIBAVFORMAT_VERSION_INT >= AV_VERSION_INT(53,2,0)
    err = avformat_open_input(&m_pFormatContext, sFilename.c_str(), 0, 0);
#else
    AVFormatParameters params;
    memset(&params, 0, sizeof(params));
    err = av_open_input_file(&m_pFormatContext, sFilename.c_str(), 
            0, 0, &params);
#endif
    if (err < 0) {
        m_sFilename = "";
        m_pFormatContext = 0;
        avcodecError(sFilename, err);
    }
    
#if LIBAVCODEC_VERSION_INT > AV_VERSION_INT(53, 8, 0)
    err = avformat_find_stream_info(m_pFormatContext, 0);
#else
    err = av_find_stream_info(m_pFormatContext);
#endif

    if (err < 0) {
        m_sFilename = "";
        m_pFormatContext = 0;
        throw Exception(AVG_ERR_VIDEO_INIT_FAILED, 
                sFilename + ": Could not find codec parameters.");
    }
    if (strcmp(m_pFormatContext->iformat->name, "image2") == 0) {
        m_sFilename = "";
        m_pFormatContext = 0;
        throw Exception(AVG_ERR_VIDEO_INIT_FAILED, 
                sFilename + ": Image files not supported as videos.");
    }
    av_read_play(m_pFormatContext);
    
    // Find audio and video streams in the file
    m_VStreamIndex = -1;
    m_AStreamIndex = -1;
    for (unsigned i = 0; i < m_pFormatContext->nb_streams; i++) {
        AVCodecContext* pContext = m_pFormatContext->streams[i]->codec;
        switch (pContext->codec_type) {
            case AVMEDIA_TYPE_VIDEO:
                if (m_VStreamIndex < 0) {
                    m_VStreamIndex = i;
                }
                break;
            case AVMEDIA_TYPE_AUDIO:
                // Ignore the audio stream if we're using sync demuxing. 
                if (m_AStreamIndex < 0 && bThreadedDemuxer && bEnableSound) {
                    m_AStreamIndex = i;
                }
                break;
            default:
                break;
        }
    }
    
    // Enable video stream demuxing
    if (m_VStreamIndex >= 0) {
        m_pVStream = m_pFormatContext->streams[m_VStreamIndex];
        m_State = OPENED;
        
        // Set video parameters
        m_TimeUnitsPerSecond = float(1.0/av_q2d(m_pVStream->time_base));
        if (m_bUseStreamFPS) {
            m_FPS = getNominalFPS();
        }
        m_Size = IntPoint(m_pVStream->codec->width, m_pVStream->codec->height);
        m_bFirstPacket = true;
        m_sFilename = sFilename;
        m_LastVideoFrameTime = -1;

        int rc = openCodec(m_VStreamIndex, bUseHardwareAcceleration);
        if (rc == -1) {
            m_VStreamIndex = -1;
            char szBuf[256];
            avcodec_string(szBuf, sizeof(szBuf), m_pVStream->codec, 0);
            m_pVStream = 0;
            throw Exception(AVG_ERR_VIDEO_INIT_FAILED, 
                    sFilename + ": unsupported video codec ("+szBuf+").");
        }
        m_PF = calcPixelFormat(true);
        m_bVideoSeekDone = false;
    }
    // Enable audio stream demuxing.
    if (m_AStreamIndex >= 0) {
        m_pAStream = m_pFormatContext->streams[m_AStreamIndex];
        m_pCurAudioPacket = 0;
        m_pTempAudioPacket = 0;
        m_LastAudioFrameTime = 0;
        m_AudioStartTimestamp = 0;
        
        if (m_pAStream->start_time != (long long)AV_NOPTS_VALUE) {
            m_AudioStartTimestamp = float(av_q2d(m_pAStream->time_base)
                    *m_pAStream->start_time);
        }
        m_EffectiveSampleRate = (int)(m_pAStream->codec->sample_rate);
        int rc = openCodec(m_AStreamIndex, false);
        if (rc == -1) {
            m_AStreamIndex = -1;
            char szBuf[256];
            avcodec_string(szBuf, sizeof(szBuf), m_pAStream->codec, 0);
            m_pAStream = 0; 
            throw Exception(AVG_ERR_VIDEO_INIT_FAILED, 
                    sFilename + ": unsupported audio codec ("+szBuf+").");
        }
        if (m_pAStream->codec->sample_fmt != SAMPLE_FMT_S16) {
            m_AStreamIndex = -1;
            m_pAStream = 0; 
            throw Exception(AVG_ERR_VIDEO_INIT_FAILED, 
                    sFilename + ": unsupported sample format (!= S16).");
        }
    }

    m_State = OPENED;
}

void FFMpegDecoder::startDecoding(bool bDeliverYCbCr, const AudioParams* pAP)
{
    AVG_ASSERT(m_State == OPENED);
    if (m_VStreamIndex >= 0) {
        m_PF = calcPixelFormat(bDeliverYCbCr);
    }
    bool bAudioEnabled = (pAP && m_bThreadedDemuxer);
    if (bAudioEnabled) {
        m_AP = *pAP;
    } else {
        m_AStreamIndex = -1;
        if (m_pAStream) {
            avcodec_close(m_pAStream->codec);
        }
        m_pAStream = 0;
    }

    if (m_AStreamIndex >= 0) {
        if (m_pAStream->codec->channels > m_AP.m_Channels) {
            throw Exception(AVG_ERR_VIDEO_INIT_FAILED, 
                    m_sFilename + ": unsupported number of audio channels (" + 
                            toString(m_pAStream->codec->channels) + ").");
            m_AStreamIndex = -1;
            m_pAStream = 0; 
        }
    }

    if (m_VStreamIndex < 0 && m_AStreamIndex < 0) {
        throw Exception(AVG_ERR_VIDEO_INIT_FAILED, 
                m_sFilename + " does not contain any valid audio or video streams.");
    }

    // Create demuxer
    AVG_ASSERT(!m_pDemuxer);
    vector<int> streamIndexes;
    if (m_VStreamIndex >= 0) {
        streamIndexes.push_back(m_VStreamIndex);
    }
    if (m_AStreamIndex >= 0) {
        streamIndexes.push_back(m_AStreamIndex);
    }
    if (m_bThreadedDemuxer) {
        m_pDemuxer = new AsyncDemuxer(m_pFormatContext, streamIndexes);
    } else {
        m_pDemuxer = new FFMpegDemuxer(m_pFormatContext, streamIndexes);
    }
    
    m_State = DECODING;
    m_SeekTime = -1;
}

void FFMpegDecoder::close() 
{
    mutex::scoped_lock lock(s_OpenMutex);
    AVG_TRACE(Logger::MEMORY, "Closing " << m_sFilename);
    
    delete m_pDemuxer;
    m_pDemuxer = 0;
    
    // Close audio and video codecs
    if (m_pVStream) {
        avcodec_close(m_pVStream->codec);
        m_pVStream = 0;
        m_VStreamIndex = -1;
    }

    if (m_pAStream) {
        avcodec_close(m_pAStream->codec);
        deleteCurAudioPacket();
        
        if (m_pAudioResampleContext) {
            audio_resample_close(m_pAudioResampleContext);
            m_pAudioResampleContext = 0;
        }

        m_LastAudioFrameTime = 0;
        m_AudioStartTimestamp = 0;
        
        m_pAStream = 0;
        m_AStreamIndex = -1;
    }
    if (m_pFormatContext) {
#if LIBAVCODEC_VERSION_INT > AV_VERSION_INT(53, 21, 0)
        avformat_close_input(&m_pFormatContext);
#else
        av_close_input_file(m_pFormatContext);
        m_pFormatContext = 0;
#endif
    }
    
    if (m_pSwsContext) {
        sws_freeContext(m_pSwsContext);
        m_pSwsContext = 0;
    }
    m_State = CLOSED;
}

VideoDecoder::DecoderState FFMpegDecoder::getState() const
{
    return m_State;
}

VideoInfo FFMpegDecoder::getVideoInfo() const
{
    AVG_ASSERT(m_State != CLOSED);
    float duration = 0;
    if (m_pVStream || m_pAStream) {
        duration = getDuration();
    }
    VideoInfo info(m_pFormatContext->iformat->name, duration, m_pFormatContext->bit_rate,
            m_pVStream != 0, m_pAStream != 0);
    if (m_pVStream) {
        info.setVideoData(m_Size, getStreamPF(), getNumFrames(), getNominalFPS(), m_FPS,
                m_pVStream->codec->codec->name, usesVDPAU(), getDuration(SS_VIDEO));
    }
    if (m_pAStream) {
        AVCodecContext * pACodec = m_pAStream->codec;
        info.setAudioData(pACodec->codec->name, pACodec->sample_rate,
                pACodec->channels, getDuration(SS_AUDIO));
    }
    return info;
}

void FFMpegDecoder::seek(float destTime) 
{
    AVG_ASSERT(m_State == DECODING);
    if (m_bFirstPacket && m_pVStream) {
        AVFrame frame;
        readFrame(frame);
    }
    m_pDemuxer->seek(destTime + getStartTime());
    m_SeekTime = destTime;
    m_bVideoEOF = false;
    m_bAudioEOF = false;
}

void FFMpegDecoder::loop()
{
    seek(0);
}

IntPoint FFMpegDecoder::getSize() const
{
    AVG_ASSERT(m_State != CLOSED);
    return m_Size;
}

int FFMpegDecoder::getCurFrame() const
{
    AVG_ASSERT(m_State != CLOSED);
    return int(m_LastVideoFrameTime*getNominalFPS()+0.5);
}

int FFMpegDecoder::getNumFramesQueued() const
{
    return 0;
}

float FFMpegDecoder::getCurTime(StreamSelect stream) const
{
    AVG_ASSERT(m_State != CLOSED);
    switch(stream) {
        case SS_DEFAULT:
        case SS_VIDEO:
            AVG_ASSERT(m_pVStream);
            return m_LastVideoFrameTime;
        case SS_AUDIO:
            AVG_ASSERT(m_pAStream);
            return m_LastAudioFrameTime;
        default:
            return -1;
    }
}

float FFMpegDecoder::getDuration(StreamSelect streamSelect) const
{
    AVG_ASSERT(m_State != CLOSED);
    long long duration;
    AVRational time_base;
    if (streamSelect == SS_DEFAULT) {
        if (m_pVStream) {
            streamSelect = SS_VIDEO;
        } else {
            streamSelect = SS_AUDIO;
        }
    }
    if (streamSelect == SS_VIDEO) {
        duration = m_pVStream->duration;
        time_base = m_pVStream->time_base;
    } else {
        duration = m_pAStream->duration;
        time_base = m_pAStream->time_base;
    }
    if (duration == (long long)AV_NOPTS_VALUE) {
        return 0;
    } else {
        return float(duration)*float(av_q2d(time_base));
    }
}

float FFMpegDecoder::getNominalFPS() const
{
    AVG_ASSERT(m_State != CLOSED);
    return float(av_q2d(m_pVStream->r_frame_rate));
}

float FFMpegDecoder::getFPS() const
{
    AVG_ASSERT(m_State != CLOSED);
    return m_FPS;
}

void FFMpegDecoder::setFPS(float fps)
{
    m_bUseStreamFPS = (fps == 0);
    if (fps == 0) {
        m_FPS = calcStreamFPS();
    } else {
        m_FPS = fps;
    }
}

float FFMpegDecoder::getVolume() const
{
    AVG_ASSERT(m_State != CLOSED);
    return m_Volume;
}

void FFMpegDecoder::setVolume(float volume)
{
    m_Volume = volume;
    if (m_State != DECODING) {
        m_LastVolume = volume;
    }
}

void copyPlaneToBmp(BitmapPtr pBmp, unsigned char * pData, int stride)
{
    unsigned char * pSrc=pData;
    unsigned char * pDest= pBmp->getPixels();
    int destStride = pBmp->getStride();
    int height = pBmp->getSize().y;
    int width = pBmp->getSize().x;
    for (int y = 0; y < height; y++) {
        memcpy(pDest, pSrc, width);
        pSrc += stride;
        pDest += destStride;
    }
}

static ProfilingZoneID RenderToBmpProfilingZone("FFMpeg: renderToBmp", true);
static ProfilingZoneID CopyImageProfilingZone("FFMpeg: copy image", true);
static ProfilingZoneID VDPAUCopyProfilingZone("FFMpeg: VDPAU copy", true);

FrameAvailableCode FFMpegDecoder::renderToBmps(vector<BitmapPtr>& pBmps, 
        float timeWanted)
{
    AVG_ASSERT(m_State == DECODING);
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
        if (pixelFormatIsPlanar(m_PF)) {
#ifdef AVG_ENABLE_VDPAU
            if (usesVDPAU()) {
                ScopeTimer timer(VDPAUCopyProfilingZone);
                vdpau_render_state* pRenderState = (vdpau_render_state *)frame.data[0];
                getPlanesFromVDPAU(pRenderState, pBmps[0], pBmps[1], pBmps[2]);
            } else {
                ScopeTimer timer(CopyImageProfilingZone);
                for (unsigned i = 0; i < pBmps.size(); ++i) {
                    copyPlaneToBmp(pBmps[i], frame.data[i], frame.linesize[i]);
                }
            }
#else 
            ScopeTimer timer(CopyImageProfilingZone);
            for (unsigned i = 0; i < pBmps.size(); ++i) {
                copyPlaneToBmp(pBmps[i], frame.data[i], frame.linesize[i]);
            }
#endif
        } else {
#ifdef AVG_ENABLE_VDPAU
            if (usesVDPAU()) {
                ScopeTimer timer(VDPAUCopyProfilingZone);
                vdpau_render_state* pRenderState = (vdpau_render_state *)frame.data[0];
                getBitmapFromVDPAU(pRenderState, pBmps[0]);
            } else {
                convertFrameToBmp(frame, pBmps[0]);
            }
#else 
            convertFrameToBmp(frame, pBmps[0]);
#endif
        }
        return FA_NEW_FRAME;
    }
    return FA_USE_LAST_FRAME;
}


#ifdef AVG_ENABLE_VDPAU
FrameAvailableCode FFMpegDecoder::renderToVDPAU(vdpau_render_state** ppRenderState)
{
    AVG_ASSERT(m_State == DECODING);
    ScopeTimer timer(RenderToBmpProfilingZone);
    AVFrame frame;
    FrameAvailableCode frameAvailable;
    readFrame(frame);
    frameAvailable = FA_NEW_FRAME;
    if (!m_bVideoEOF && frameAvailable == FA_NEW_FRAME) {
        if (usesVDPAU()) {
            ScopeTimer timer(VDPAUCopyProfilingZone);
            vdpau_render_state *pRenderState = (vdpau_render_state *)frame.data[0];
            *ppRenderState = pRenderState;
        }
        return FA_NEW_FRAME;
    }
    return FA_USE_LAST_FRAME;
}
#endif

void FFMpegDecoder::throwAwayFrame(float timeWanted)
{
    AVG_ASSERT(m_State == DECODING);
    AVFrame frame;
    readFrameForTime(frame, timeWanted);
}

bool FFMpegDecoder::isVideoSeekDone()
{
    bool bSeekDone = m_bVideoSeekDone;
    m_bVideoSeekDone = false;
    return bSeekDone;
}

bool FFMpegDecoder::isEOF(StreamSelect stream) const
{
    AVG_ASSERT(m_State == DECODING);
    switch(stream) {
        case SS_AUDIO:
            return (!m_pAStream || m_bAudioEOF);
        case SS_VIDEO:
            return (!m_pVStream || m_bVideoEOF);
        case SS_ALL:
            return isEOF(SS_VIDEO) && isEOF(SS_AUDIO);
        default:
            return false;
    }
}

void FFMpegDecoder::logConfig()
{
    bool bVDPAUAvailable = false;
#ifdef AVG_ENABLE_VDPAU
    bVDPAUAvailable = VDPAUDecoder::isAvailable();
#endif
    if (bVDPAUAvailable) {
        AVG_TRACE(Logger::CONFIG, "Hardware video acceleration: VDPAU");
    } else {
        AVG_TRACE(Logger::CONFIG, "Hardware video acceleration: Off");
    }
}

AudioBufferPtr FFMpegDecoder::getAudioBuffer()
{
    AVG_ASSERT(m_State == DECODING);
    short pDecodedData[AVCODEC_MAX_AUDIO_FRAME_SIZE/2];

    int bytesConsumed = 0;
    int bytesDecoded = 0;
    while (bytesDecoded == 0) {
        if (!m_pCurAudioPacket) {
//            cerr << "                  get new packet" << endl;
            bool bSeekDone;
            m_pCurAudioPacket = m_pDemuxer->getPacket(m_AStreamIndex, bSeekDone);
            if (!m_pCurAudioPacket) {
//                cerr << "                  eof" << endl;
                m_bAudioEOF = true;
                return AudioBufferPtr();
            }
            if (bSeekDone) {
                m_LastAudioFrameTime = m_pCurAudioPacket->dts*av_q2d(m_pAStream->time_base)
                        - m_AudioStartTimestamp;
            }
//            cerr << "                  packet size: " << m_pCurAudioPacket->size << endl;
            m_pTempAudioPacket = new AVPacket;
            av_init_packet(m_pTempAudioPacket);
            m_pTempAudioPacket->data = m_pCurAudioPacket->data;
            m_pTempAudioPacket->size = m_pCurAudioPacket->size;
        }
        bytesDecoded = AVCODEC_MAX_AUDIO_FRAME_SIZE;
#if LIBAVCODEC_VERSION_INT > AV_VERSION_INT(52, 31, 0)
        bytesConsumed = avcodec_decode_audio3(m_pAStream->codec, pDecodedData,
                &bytesDecoded, m_pTempAudioPacket);
#else
        bytesConsumed = avcodec_decode_audio2(m_pAStream->codec, pDecodedData,
                &bytesDecoded, m_pTempAudioPacket->data, m_pTempAudioPacket->size);
#endif
//        cerr << "                  avcodec_decode_audio: bytesConsumed=" <<
//                  bytesConsumed << ", bytesDecoded=" << bytesDecoded << endl;
        if (bytesConsumed < 0) {
            // Error decoding -> throw away current packet.
            bytesDecoded = 0;
            deleteCurAudioPacket();
//            cerr << "                  error decoding" << endl;
        } else {
            m_pTempAudioPacket->data += bytesConsumed;
            m_pTempAudioPacket->size -= bytesConsumed;

            if (m_pTempAudioPacket->size <= 0) {
                deleteCurAudioPacket();
            }
        }
    }
    int framesDecoded = bytesDecoded/(m_pAStream->codec->channels*sizeof(short));
    AudioBufferPtr pBuffer;
    bool bNeedsResample = (m_EffectiveSampleRate != m_AP.m_SampleRate || 
            m_pAStream->codec->channels != m_AP.m_Channels);
    if (bNeedsResample) {
        pBuffer = resampleAudio(pDecodedData, framesDecoded);
    } else {
        pBuffer = AudioBufferPtr(new AudioBuffer(framesDecoded, m_AP));
        memcpy(pBuffer->getData(), pDecodedData, bytesDecoded);
    }
    m_LastAudioFrameTime += float(pBuffer->getNumFrames())/m_AP.m_SampleRate;
//    cerr << "                  Decoder time: " << m_LastAudioFrameTime << endl;
    pBuffer->volumize(m_LastVolume, m_Volume);
    m_LastVolume = m_Volume;
//    cerr << "                FFMpegDecoder::getAudioBuffer() end" << endl;
    return pBuffer;
}

PixelFormat FFMpegDecoder::calcPixelFormat(bool bUseYCbCr)
{
    AVCodecContext const* pContext = getCodecContext();
    if (bUseYCbCr) {
        switch(pContext->pix_fmt) {
            case PIX_FMT_YUV420P:
#ifdef AVG_ENABLE_VDPAU
            case PIX_FMT_VDPAU_H264:
            case PIX_FMT_VDPAU_MPEG1:
            case PIX_FMT_VDPAU_MPEG2:
            case PIX_FMT_VDPAU_WMV3:
            case PIX_FMT_VDPAU_VC1:
#endif
                return YCbCr420p;
            case PIX_FMT_YUVJ420P:
                return YCbCrJ420p;
            case PIX_FMT_YUVA420P:
                return YCbCrA420p;
            default:
                break;
        }
    }
    bool bAlpha = (pContext->pix_fmt == PIX_FMT_BGRA ||
            pContext->pix_fmt == PIX_FMT_YUVA420P);
    return BitmapLoader::get()->getDefaultPixelFormat(bAlpha);
}

static ProfilingZoneID ConvertImageLibavgProfilingZone(
        "FFMpeg: colorspace conv (libavg)", true);
static ProfilingZoneID ConvertImageSWSProfilingZone(
        "FFMpeg: colorspace conv (SWS)", true);
static ProfilingZoneID SetAlphaProfilingZone("FFMpeg: set alpha channel", true);

void FFMpegDecoder::convertFrameToBmp(AVFrame& frame, BitmapPtr pBmp)
{
    AVPicture destPict;
    unsigned char * pDestBits = pBmp->getPixels();
    destPict.data[0] = pDestBits;
    destPict.linesize[0] = pBmp->getStride();
    ::PixelFormat destFmt;
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
            AVG_ASSERT_MSG(false, (string("FFMpegDecoder: Dest format ") +
                    toString(pBmp->getPixelFormat()) + " not supported.").c_str());
            destFmt = PIX_FMT_BGRA;
    }
    AVCodecContext const* pContext = getCodecContext();
    {
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
#ifdef AVG_ENABLE_VDPAU
        } else if (destFmt == PIX_FMT_BGRA && usesVDPAU()) {
            vdpau_render_state *pRenderState = (vdpau_render_state *)frame.data[0];
            getBitmapFromVDPAU(pRenderState, pBmp);
#endif
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
            if (pBmp->getPixelFormat() == B8G8R8X8 || pBmp->getPixelFormat() == R8G8B8X8)
            {
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
}
       
PixelFormat FFMpegDecoder::getPixelFormat() const
{
    AVG_ASSERT(m_State != CLOSED);
    return m_PF;
}

void FFMpegDecoder::initVideoSupport()
{
    if (!s_bInitialized) {
        av_register_all();
        s_bInitialized = true;
        // Tune libavcodec console spam.
//        av_log_set_level(AV_LOG_DEBUG);
        av_log_set_level(AV_LOG_QUIET);
    }
}

bool FFMpegDecoder::usesVDPAU() const
{
#ifdef AVG_ENABLE_VDPAU
    AVCodecContext const* pContext = getCodecContext();
    return pContext->codec && (pContext->codec->capabilities & CODEC_CAP_HWACCEL_VDPAU);
#else
    return false;
#endif
}

int FFMpegDecoder::getNumFrames() const
{
    AVG_ASSERT(m_State != CLOSED);
    // This is broken for some videos, but the code here is correct.
    // So fix ffmpeg :-).
    int numFrames =  int(m_pVStream->nb_frames);
    if (numFrames > 0) {
        return numFrames;
    } else {
        return int(getDuration() * calcStreamFPS());
    }
}

FrameAvailableCode FFMpegDecoder::readFrameForTime(AVFrame& frame, float timeWanted)
{
    AVG_ASSERT(m_State == DECODING);
//    cerr << "        readFrameForTime " << timeWanted << ", LastFrameTime= " 
//            << m_LastVideoFrameTime << ", diff= " << m_LastVideoFrameTime-timeWanted 
//            << endl;
    AVG_ASSERT(timeWanted != -1);
    float timePerFrame = 1.0f/m_FPS;
    if (timeWanted-m_LastVideoFrameTime < 0.5f*timePerFrame && m_SeekTime == -1) {
//        cerr << "DISPLAY AGAIN." << endl;
        // The last frame is still current. Display it again.
        return FA_USE_LAST_FRAME;
    } else {
        bool bInvalidFrame = true;
        while (bInvalidFrame && !m_bVideoEOF) {
            float frameTime = readFrame(frame);
            bInvalidFrame = frameTime-timeWanted < -0.5f*timePerFrame;
#if AVG_ENABLE_VDPAU
            if (usesVDPAU() && bInvalidFrame && !m_bVideoEOF) {
                vdpau_render_state *pRenderState = (vdpau_render_state *)frame.data[0];
                unlockVDPAUSurface(pRenderState);
            }
#endif
//            cerr << "        readFrame returned time " << frameTime << ", diff= " <<
//                    frameTime-timeWanted <<  endl;
        }
//        cerr << "NEW FRAME." << endl;
    }
    return FA_NEW_FRAME;
}

static ProfilingZoneID DecodeProfilingZone("FFMpeg: decode", true);

float FFMpegDecoder::readFrame(AVFrame& frame)
{
    AVG_ASSERT(m_State == DECODING);
    ScopeTimer timer(DecodeProfilingZone); 

    if (m_bEOFPending) {
        m_bVideoEOF = true;
        m_bEOFPending = false;
        return m_LastVideoFrameTime;
    }
    AVCodecContext* pContext = getCodecContext();
    int bGotPicture = 0;
    AVPacket* pPacket = 0;
    float frameTime = -1;
    while (!bGotPicture && !m_bVideoEOF) {
        bool bSeekDone;
        pPacket = m_pDemuxer->getPacket(m_VStreamIndex, bSeekDone);
        if (bSeekDone) {
            m_LastVideoFrameTime = m_SeekTime-1.0f/m_FPS;
            m_SeekTime = -1;
            m_bVideoSeekDone = true;
        }
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
            else {
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
    }
    AVG_ASSERT(frameTime != -1)
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

float FFMpegDecoder::getFrameTime(long long dts)
{
    if (m_VideoStartTimestamp == -1) {
        m_VideoStartTimestamp = dts;
    }
    float frameTime;
    if (m_bUseStreamFPS) {
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

float FFMpegDecoder::getStartTime()
{
    if (m_pVStream) {
        return m_VideoStartTimestamp/m_TimeUnitsPerSecond;
    } else {
        return m_AudioStartTimestamp;
    }
}

float FFMpegDecoder::calcStreamFPS() const
{
    return (float(m_pVStream->r_frame_rate.num)/m_pVStream->r_frame_rate.den);
}

string FFMpegDecoder::getStreamPF() const
{
    AVCodecContext const* pCodec = getCodecContext();
    ::PixelFormat pf = pCodec->pix_fmt;
    const char* psz = av_get_pix_fmt_name(pf);
    string s;
    if (psz) {
        s = psz;
    } 
    return s;
}

AudioBufferPtr FFMpegDecoder::resampleAudio(short* pDecodedData, int framesDecoded)
{
    if (!m_pAudioResampleContext) {
//        cerr << "init resample" << endl;
#if LIBAVFORMAT_VERSION_INT >= AV_VERSION_INT(52, 24, 0)
        m_pAudioResampleContext = av_audio_resample_init(m_AP.m_Channels, 
                m_pAStream->codec->channels, m_AP.m_SampleRate, m_EffectiveSampleRate,
                SAMPLE_FMT_S16, SAMPLE_FMT_S16, 16, 10, 0, 0.8);
#else
        m_pAudioResampleContext = audio_resample_init(m_AP.m_Channels, 
                m_pAStream->codec->channels, m_AP.m_SampleRate, m_EffectiveSampleRate);
#endif        
    }

    short pResampledData[AVCODEC_MAX_AUDIO_FRAME_SIZE/2];
    int framesResampled = audio_resample(m_pAudioResampleContext, pResampledData,
            pDecodedData, framesDecoded);
    AudioBufferPtr pBuffer(new AudioBuffer(framesResampled, m_AP));
    memcpy(pBuffer->getData(), pResampledData, 
            framesResampled*m_AP.m_Channels*sizeof(short));
//    cerr << "Resample: " << framesDecoded << "->" << framesResampled << endl;
    return pBuffer;
}

void FFMpegDecoder::deleteCurAudioPacket()
{
    if (m_pCurAudioPacket) {
        av_free_packet(m_pCurAudioPacket);
        delete m_pCurAudioPacket;
        m_pCurAudioPacket = 0;
        delete m_pTempAudioPacket;
        m_pTempAudioPacket = 0;
    }
}

AVCodecContext const* FFMpegDecoder::getCodecContext() const
{
    return m_pVStream->codec;
}

AVCodecContext* FFMpegDecoder::getCodecContext()
{
    return m_pVStream->codec;
}

}

