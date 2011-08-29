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

#include "FFMpegDemuxer.h"
#include "../base/ScopeTimer.h"
#include "../base/ObjectCounter.h"
#include "../base/Exception.h"

#include <cstring>
#include <iostream>

using namespace std;

namespace avg {

FFMpegDemuxer::FFMpegDemuxer(AVFormatContext * pFormatContext, vector<int> streamIndexes)
    : m_pFormatContext(pFormatContext)
{
    ObjectCounter::get()->incRef(&typeid(*this));
    for (unsigned i = 0; i < streamIndexes.size(); ++i) {
        m_PacketLists[streamIndexes[i]] = PacketList();
    }
}

FFMpegDemuxer::~FFMpegDemuxer()
{
    clearPacketCache();
    ObjectCounter::get()->decRef(&typeid(*this));
}

AVPacket * FFMpegDemuxer::getPacket(int streamIndex)
{
    // Make sure enableStream was called on streamIndex.
    AVG_ASSERT(m_PacketLists.size() > 0);
    AVG_ASSERT(streamIndex > -1 && streamIndex < 10);
    if (m_PacketLists.find(streamIndex) == m_PacketLists.end()) {
        cerr << this << ": getPacket: Stream " << streamIndex << " not found." << endl;
        dump();
        AVG_ASSERT(false);
    }
    PacketList & CurPacketList = m_PacketLists.find(streamIndex)->second;
    AVPacket * pPacket;
    if (!CurPacketList.empty()) {
        pPacket = CurPacketList.front();
        CurPacketList.pop_front();
    } else {
        do {
            pPacket = new AVPacket;
            memset(pPacket, 0, sizeof(AVPacket));
            int err = av_read_frame(m_pFormatContext, pPacket);
            // TODO: Check url_ferror here too.
            if (err < 0) {
                av_free_packet(pPacket);
                delete pPacket;
                pPacket = 0;
                return 0;
            }
            if (pPacket->stream_index != streamIndex) {
                if (m_PacketLists.find(pPacket->stream_index) != m_PacketLists.end()) {
                    av_dup_packet(pPacket);
                    PacketList& OtherPacketList = 
                            m_PacketLists.find(pPacket->stream_index)->second;
                    OtherPacketList.push_back(pPacket);
                } else {
                    av_free_packet(pPacket);
                    delete pPacket;
                    pPacket = 0;
                } 
            } else {
                av_dup_packet(pPacket);
            }
        } while (!pPacket || pPacket->stream_index != streamIndex);
    }

//    double timeBase = av_q2d(m_pFormatContext->streams[streamIndex]->time_base);
//    cerr << "FFMpegDemuxer: " << streamIndex << ": " << pPacket->dts*timeBase << endl;
    return pPacket;
}

void FFMpegDemuxer::seek(double destTime)
{
#if LIBAVFORMAT_BUILD <= 4616
    av_seek_frame(m_pFormatContext, -1, destTime*1000000);
#else
#if LIBAVFORMAT_BUILD < ((49<<16)+(0<<8)+0)
    av_seek_frame(m_pFormatContext, -1, destTime*1000000, 0);
#else
    av_seek_frame(m_pFormatContext, -1, (long long)(destTime*AV_TIME_BASE), AVSEEK_FLAG_BACKWARD);
#endif
#endif
    clearPacketCache();
    map<int, PacketList>::iterator it;
    for (it = m_PacketLists.begin(); it != m_PacketLists.end(); ++it) {
        int CurStreamIndex = it->first;
        AVStream * pStream = m_pFormatContext->streams[CurStreamIndex];
        avcodec_flush_buffers(pStream->codec);
    }
}

void FFMpegDemuxer::clearPacketCache()
{
    map<int, PacketList>::iterator it;
    for (it = m_PacketLists.begin(); it != m_PacketLists.end(); ++it) {
        PacketList::iterator it2;
        PacketList* thePacketList = &(it->second);
        for (it2 = thePacketList->begin(); it2 != thePacketList->end(); ++it2) {
            av_free_packet(*it2);
            delete *it2;
        }
        thePacketList->clear();
    }
}

void FFMpegDemuxer::dump()
{
    map<int, PacketList>::iterator it;
    cerr << "FFMpegDemuxer " << this << endl;
    cerr << "packetlists.size(): " << int(m_PacketLists.size()) << endl;
    for (it = m_PacketLists.begin(); it != m_PacketLists.end(); ++it) {
        cerr << "  " << it->first << ":  " << int(it->second.size()) << endl;
    }
}

}
