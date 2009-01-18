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

#ifndef _Image_H_
#define _Image_H_

#include "../api.h"
#include "RasterNode.h"

#include "../graphics/Bitmap.h"

#include <string>

namespace avg {

class AVG_API Image : public RasterNode
{
    public:
        static NodeDefinition createDefinition();
        
        Image (const ArgList& Args, bool bFromXML);
        virtual ~Image ();
        virtual void setRenderingEngines(DisplayEngine * pDisplayEngine, 
                AudioEngine * pAudioEngine);
        virtual void connect();
        virtual void disconnect();

        const std::string& getHRef() const;
        void setHRef(const std::string& href);
        int getHue() const
        {
            return m_Hue;
        }
        int getSaturation() const
        {
            return m_Saturation;
        }
        void setBitmap(const Bitmap * pBmp);
        
        virtual void render(const DRect& Rect);
        virtual void checkReload();
        
        virtual Bitmap* getBitmap();
        virtual IntPoint getMediaSize();

    private:
        void load();
        void setupSurface();
        std::string m_Filename;
        std::string m_href;
        BitmapPtr m_pBmp;
        bool m_bIsImageAvailable;
    
        int m_Hue;
        int m_Saturation;
};

}

#endif

