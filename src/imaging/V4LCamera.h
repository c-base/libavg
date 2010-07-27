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

#ifndef _V4LCamera_H_
#define _V4LCamera_H_

#include "../api.h"
#include "../avgconfigwrapper.h"

#include "Camera.h"
#include <string>
#include <vector>

namespace avg {

typedef unsigned int V4LCID_t;

class AVG_API V4LCamera: public Camera {

    struct Buffer {
        void * start;
        size_t length;
    };
    
    public:
        V4LCamera(std::string sDevice, int Channel, IntPoint Size,
                PixelFormat camPF, PixelFormat destPF, double FrameRate);
        virtual ~V4LCamera();

        virtual IntPoint getImgSize();
        virtual BitmapPtr getImage(bool bWait);
        virtual bool isCameraAvailable();

        virtual const std::string& getDevice() const; 
        virtual const std::string& getDriverName() const; 
        virtual double getFrameRate() const;
        
        virtual int getFeature(CameraFeature Feature) const;
        virtual void setFeature(CameraFeature Feature, int Value, 
                bool bIgnoreOldValue=false);
        virtual void setFeatureOneShot(CameraFeature Feature);
        virtual int getWhitebalanceU() const;
        virtual int getWhitebalanceV() const;
        virtual void setWhitebalance(int u, int v, bool bIgnoreOldValue=false);
       
        static void dumpCameras();

    private:
        void initDevice();
        void startCapture();
        void initMMap();
        virtual void close();
        
        int getV4LPF(PixelFormat pf);
        
        int m_Fd;
        int m_Channel;
        std::string m_sDevice;
        std::string m_sDriverName;
        std::vector<Buffer> m_vBuffers;
        bool m_bCameraAvailable;
        int m_v4lPF;
        IntPoint m_ImgSize;
        double m_FrameRate;
        
        void setFeature(V4LCID_t V4LFeature, int Value);
        V4LCID_t getFeatureID(CameraFeature Feature) const;
        std::string getFeatureName(V4LCID_t V4LFeature);
        bool isFeatureSupported(V4LCID_t V4LFeature) const;
        typedef std::map<V4LCID_t, unsigned int> FeatureMap;
        typedef std::map<int, std::string> FeatureNamesMap;
        FeatureMap m_Features;
        // TODO: Feature strings should really be handled by 
        //       Camera::cameraFeatureToString
        FeatureNamesMap m_FeaturesNames; 
};

}

#endif
