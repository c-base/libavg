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

#ifndef _AudioSource_H_
#define _AudioSource_H_

#include "../api.h"

#include "AudioMsg.h"

#include <boost/shared_ptr.hpp>

namespace avg
{

class AVG_API AudioSource
{
public:
    AudioSource(AudioMsgQueue& msgQ, AudioMsgQueue& statusQ, int sampleRate);
    virtual ~AudioSource();

    void pause();
    void play();
    void notifySeek();
    void setVolume(float volume);

    void fillAudioBuffer(AudioBufferPtr pBuffer);

private:
    bool processNextMsg(bool bWait);

    AudioMsgQueue& m_MsgQ;    
    AudioMsgQueue& m_StatusQ;
    int m_SampleRate;
    AudioBufferPtr m_pInputAudioBuffer;
    float m_LastTime;
    int m_CurInputAudioPos;
    bool m_bPaused;
    bool m_bSeeking;
    float m_Volume;
    float m_LastVolume;
};

typedef boost::shared_ptr<AudioSource> AudioSourcePtr;

}

#endif
