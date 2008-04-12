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

#include "AsyncDemuxer.h"
#include "../base/ScopeTimer.h"
#include "../base/ObjectCounter.h"

#include <boost/bind.hpp>

#include <iostream>

using namespace std;

typedef boost::mutex::scoped_lock scoped_lock;

namespace avg {

AsyncDemuxer::AsyncDemuxer(AVFormatContext * pFormatContext)
    : m_pCmdQ(new VideoDemuxerThread::CmdQueue),
      m_bSeekPending(false)
{
    ObjectCounter::get()->incRef(&typeid(*this));
    m_pSyncDemuxer = IDemuxerPtr(new FFMpegDemuxer(pFormatContext));
    m_pDemuxThread = new boost::thread(VideoDemuxerThread(*m_pCmdQ, pFormatContext));
}

AsyncDemuxer::~AsyncDemuxer()
{
    if (m_pDemuxThread) {
        waitForSeekDone();
        m_pCmdQ->push(Command<VideoDemuxerThread>(boost::bind(
                &VideoDemuxerThread::stop, _1)));
        map<int, VideoPacketQueuePtr>::iterator it;
        for (it=m_PacketQs.begin(); it != m_PacketQs.end(); ++it) {
            // If the Queue is full, this breaks the lock in the thread.
            try {
                PacketVideoMsgPtr pPacketMsg;
                pPacketMsg = it->second->pop(false);
                pPacketMsg->freePacket();
            } catch (Exception&) {
                // This gets thrown if the queue is empty.
            }
        }
        m_pDemuxThread->join();
        delete m_pDemuxThread;
        m_pDemuxThread = 0;
        for (it=m_PacketQs.begin(); it != m_PacketQs.end(); it++) {
            VideoPacketQueuePtr pPacketQ = it->second;
            PacketVideoMsgPtr pPacketMsg;
            try {
                while(true) {
                    pPacketMsg = pPacketQ->pop(false);
                    pPacketMsg->freePacket();
                }
            } catch (Exception&) {
                // This gets thrown if the queue is empty.
            }
        }
    }
    ObjectCounter::get()->decRef(&typeid(*this));
}

void AsyncDemuxer::enableStream(int StreamIndex)
{
    VideoPacketQueuePtr pPacketQ(new VideoPacketQueue(100));
    m_PacketQs[StreamIndex] = pPacketQ;
    m_bSeekDone[StreamIndex] = true;
    m_pCmdQ->push(Command<VideoDemuxerThread>(boost::bind(
                &VideoDemuxerThread::enableStream, _1, pPacketQ, StreamIndex)));
}

AVPacket * AsyncDemuxer::getPacket(int StreamIndex)
{
    waitForSeekDone();
    // TODO: This blocks if there is no packet. Is that ok?
    PacketVideoMsgPtr pPacketMsg = m_PacketQs[StreamIndex]->pop(true);
    assert (!pPacketMsg->isSeekDone());

    return pPacketMsg->getPacket();
}

void AsyncDemuxer::seek(long long DestTime)
{
    waitForSeekDone();
    scoped_lock Lock(m_SeekMutex);
    m_pCmdQ->push(Command<VideoDemuxerThread>(boost::bind(
                &VideoDemuxerThread::seek, _1, DestTime)));
    m_bSeekPending = true;
    bool bAllSeeksDone = true;
    map<int, VideoPacketQueuePtr>::iterator it;
    for (it=m_PacketQs.begin(); it != m_PacketQs.end(); it++) {
        VideoPacketQueuePtr pPacketQ = it->second;
        PacketVideoMsgPtr pPacketMsg;
        map<int, bool>::iterator itSeekDone = m_bSeekDone.find(it->first);
        itSeekDone->second = false;
        try {
            while(!itSeekDone->second) {
                pPacketMsg = pPacketQ->pop(false);
                itSeekDone->second = pPacketMsg->isSeekDone();
                pPacketMsg->freePacket();
            }
            itSeekDone->second = true;
        } catch (Exception&) {
            // This gets thrown if the queue is empty.
            bAllSeeksDone = false;
        }
    }
    if (bAllSeeksDone) {
        m_bSeekPending = false;
    }
}

void AsyncDemuxer::waitForSeekDone()
{
    scoped_lock Lock(m_SeekMutex);
    if (m_bSeekPending) {
        m_bSeekPending = false;
        map<int, VideoPacketQueuePtr>::iterator it;
        for (it=m_PacketQs.begin(); it != m_PacketQs.end(); it++) {
            VideoPacketQueuePtr pPacketQ = it->second;
            PacketVideoMsgPtr pPacketMsg;
            map<int, bool>::iterator itSeekDone = m_bSeekDone.find(it->first);
            while (!itSeekDone->second) {
                pPacketMsg = pPacketQ->pop(true);
                itSeekDone->second = pPacketMsg->isSeekDone();
                pPacketMsg->freePacket();
            }
        }
    }
}

}
