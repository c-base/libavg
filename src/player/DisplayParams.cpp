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

#include "DisplayParams.h"

#include <iostream>

using namespace std;

namespace avg {

DisplayParams::DisplayParams()
    : m_bFullscreen(false),
      m_BPP(24),
      m_bShowCursor(true),
      m_VBRate(1),
      m_Framerate(0)
{ 
    m_Gamma[0] = -1.0f;
    m_Gamma[1] = -1.0f;
    m_Gamma[2] = -1.0f;
    m_Windows.push_back(WindowParams());
}

DisplayParams::~DisplayParams()
{
}

void DisplayParams::resetWindows()
{
    m_Windows.clear();
    m_Windows.push_back(WindowParams());
}

void DisplayParams::dump() const
{
    cerr << "DisplayParams: " << endl;
    cerr << "  fullscreen: " << m_bFullscreen << endl;
    cerr << "  bpp: " << m_BPP << endl;
    cerr << "  show cursor: " << m_bShowCursor << endl;
    cerr << "  vbrate: " << m_VBRate << endl;
    cerr << "  framerate: " << m_Framerate << endl;
    for (unsigned i=0; i<m_Windows.size(); ++i) {
        m_Windows[i].dump();
    }
}

}

