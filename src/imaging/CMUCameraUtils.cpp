//
//  libavg - Media Playback Engine. 
//  Copyright (C) 2003-2014 Ulrich von Zadow
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
//  Original author of this file is Nick Hebner (hebnern@gmail.com).
//

#include "CMUCameraUtils.h"

#include "../base/Logger.h"
#include "../base/Exception.h"
#include "../base/StringHelper.h"

namespace avg {

using namespace std;

#define FORMAT_0 0
// Format 0 Modes
#define MODE_160_120_YUV444  0
#define MODE_320x240_YUV422  1
#define MODE_640x480_YUV411  2
#define MODE_640x480_YUV422  3
#define MODE_640x480_RGB     4
#define MODE_640x480_MONO    5
#define MODE_640x480_MONO16  6

#define FORMAT_1 1
// Format 1 Modes
#define MODE_800x600_YUV422  0
#define MODE_800x600_RGB     1
#define MODE_800x600_MONO    2
#define MODE_1024x768_YUV422 3
#define MODE_1024x768_RGB    4
#define MODE_1024x768_MONO   5
#define MODE_800x600_MONO16  6
#define MODE_1024x768_MONO16 7

#define FORMAT_2 2
#define MODE_1280x960_YUV422  0
#define MODE_1280x960_RGB     1
#define MODE_1280x960_MONO    2
#define MODE_1600x1200_YUV422 3
#define MODE_1600x1200_RGB    4
#define MODE_1600x1200_MONO   5
#define MODE_1280x960_MONO16  6
#define MODE_1600x1200_MONO16 7

// Framerates
#define FRAMERATE_1_875 0
#define FRAMERATE_3_75  1
#define FRAMERATE_7_5   2
#define FRAMERATE_15    3
#define FRAMERATE_30    4
#define FRAMERATE_60    5
#define FRAMERATE_120   6
#define FRAMERATE_240   7

void getVideoFormatAndMode(IntPoint& Size, PixelFormat pf, 
        unsigned long* pVideoFormat, unsigned long* pVideoMode) 
{
    *pVideoMode = -1;
    *pVideoFormat = -1;
    if (Size.x == 320 && Size.y == 240) {
        *pVideoFormat = FORMAT_0;
        if (pf == YCbCr422) {
            *pVideoMode = MODE_320x240_YUV422;
        }
    } else if (Size.x == 640 && Size.y == 480) {
        *pVideoFormat = FORMAT_0;
        if (pf == I8 || pf == BAYER8) {
            *pVideoMode = MODE_640x480_MONO;
        } else if (pf == I16) {
            *pVideoMode = MODE_640x480_MONO16;
        } else if (pf == YCbCr411) {
            *pVideoMode = MODE_640x480_YUV411;
        } else if (pf == YCbCr422) {
            *pVideoMode = MODE_640x480_YUV422;
        } else if (pf == R8G8B8 || pf == B8G8R8) {
            *pVideoMode = MODE_640x480_RGB;
        }
    } else if (Size.x == 800 && Size.y == 600) {
        *pVideoFormat = FORMAT_1;
        if (pf == I8 || pf == BAYER8) {
            *pVideoMode = MODE_800x600_MONO;
        } else if (pf == I16) {
            *pVideoMode = MODE_800x600_MONO16;
        } else if (pf == YCbCr422) {
            *pVideoMode = MODE_800x600_YUV422;
        } else if (pf == R8G8B8 || pf == B8G8R8) {
            *pVideoMode = MODE_800x600_RGB;
        }
    } else if (Size.x == 1024 && Size.y == 768) {
        *pVideoFormat = FORMAT_1;
        if (pf == I8 || pf == BAYER8) {
            *pVideoMode = MODE_1024x768_MONO;
        } else if (pf == I16) {
            *pVideoMode = MODE_1024x768_MONO16;
        } else if (pf == YCbCr422) {
            *pVideoMode = MODE_1024x768_YUV422;
        } else if (pf == R8G8B8 || pf == B8G8R8) {
            *pVideoMode = MODE_1024x768_RGB;
        }
    } else if (Size.x == 1280 && Size.y == 960) {
        *pVideoFormat = FORMAT_2;
        if (pf == I8 || pf == BAYER8) {
            *pVideoMode = MODE_1280x960_MONO;
        } else if (pf == I16) {
            *pVideoMode = MODE_1280x960_MONO16;
        } else if (pf == YCbCr422) {
            *pVideoMode = MODE_1280x960_YUV422;
        } else if (pf == R8G8B8 || pf == B8G8R8) {
            *pVideoMode = MODE_1280x960_RGB;
        }
    } else if (Size.x == 1600 && Size.y == 1200) {
        *pVideoFormat = FORMAT_2;
        if (pf == I8 || pf == BAYER8) {
            *pVideoMode = MODE_1600x1200_MONO;
        } else if (pf == I16) {
            *pVideoMode = MODE_1600x1200_MONO16;
        } else if (pf == YCbCr422) {
            *pVideoMode = MODE_1600x1200_YUV422;
        } else if (pf == R8G8B8 || pf == B8G8R8) {
            *pVideoMode = MODE_1600x1200_RGB;
        }
    }
    if (*pVideoMode == -1 || *pVideoFormat == -1) { 
        throw Exception(AVG_ERR_INVALID_ARGS,
                "Unsupported or illegal camera mode ("+toString(Size.x)+", "+toString(Size.y)+
                "), "+getPixelFormatString(pf)+".");
    }
}

unsigned long getFrameRateConst(float frameRate)
{
    if (frameRate == 1.875f) {
        return FRAMERATE_1_875;
    } else if (frameRate == 3.75f) {
        return FRAMERATE_3_75;
    } else if (frameRate == 7.5f) {
        return FRAMERATE_7_5;
    } else if (frameRate == 15) {
        return FRAMERATE_15;
    } else if (frameRate == 30) {
        return FRAMERATE_30;
    } else if (frameRate == 60) {
        return FRAMERATE_60;
    } else if (frameRate == 120) {
        return FRAMERATE_120;
    } else if (frameRate == 240) {
        return FRAMERATE_240;
    } else {
        throw Exception(AVG_ERR_INVALID_ARGS,
                "Unsupported or illegal value ("+toString(frameRate)+
                ") for camera framerate.");
    }
}

float getFrameRateFloat(unsigned long frameRate)
{
    if(frameRate == FRAMERATE_1_875){
        return 1.875;
    } else if (frameRate == FRAMERATE_3_75){
        return 3.75;
    } else if (frameRate == FRAMERATE_7_5){
        return 7.5;
    } else if (frameRate == FRAMERATE_15){
        return 15;
    } else if (frameRate == FRAMERATE_30){
        return 30;
    } else if (frameRate == FRAMERATE_60){
        return 60;
    } else if (frameRate == FRAMERATE_120){
        return 120;
    } else if (frameRate == FRAMERATE_240){
        return 240;
    } else {
        throw Exception(AVG_ERR_INVALID_ARGS,
                "Unsupported or illegal value ("+toString(frameRate)+
                ") as camera framerate.");
    }
}

CAMERA_FEATURE getFeatureID(CameraFeature Feature)
{
    switch(Feature) {
        case CAM_FEATURE_BRIGHTNESS:
            return FEATURE_BRIGHTNESS;
        case CAM_FEATURE_EXPOSURE:
            return FEATURE_AUTO_EXPOSURE;
        case CAM_FEATURE_SHARPNESS:
            return FEATURE_SHARPNESS;
        case CAM_FEATURE_WHITE_BALANCE:
            return FEATURE_WHITE_BALANCE;
        case CAM_FEATURE_HUE:
            return FEATURE_HUE;
        case CAM_FEATURE_SATURATION:
            return FEATURE_SATURATION;
        case CAM_FEATURE_GAMMA:
            return FEATURE_GAMMA;
        case CAM_FEATURE_SHUTTER:
            return FEATURE_SHUTTER;
        case CAM_FEATURE_GAIN:
            return FEATURE_GAIN;
        case CAM_FEATURE_IRIS:
            return FEATURE_IRIS;
        case CAM_FEATURE_FOCUS:
            return FEATURE_FOCUS;
        case CAM_FEATURE_TEMPERATURE:
            return FEATURE_TEMPERATURE;
        case CAM_FEATURE_TRIGGER:
            return FEATURE_TRIGGER_MODE;
        case CAM_FEATURE_ZOOM:
            return FEATURE_ZOOM;
        case CAM_FEATURE_PAN:
            return FEATURE_PAN;
        case CAM_FEATURE_TILT:
            return FEATURE_TILT;
        case CAM_FEATURE_OPTICAL_FILTER:
            return FEATURE_OPTICAL_FILTER;
        case CAM_FEATURE_CAPTURE_SIZE:
            return FEATURE_CAPTURE_SIZE;
        case CAM_FEATURE_CAPTURE_QUALITY:
            return FEATURE_CAPTURE_QUALITY;
        default:
            return FEATURE_INVALID_FEATURE;
    }
}

void getImageSizeAndPF(unsigned long videoFormat, unsigned long videoMode,
    IntPoint &pSize, PixelFormat &pPixelFormat)
{
    int format = (int) videoFormat;
    int mode = (int) videoMode;
    switch(format) {
        case FORMAT_0: {
            if (mode == MODE_160_120_YUV444) {
                pSize = IntPoint(160,120);
                pPixelFormat = PixelFormat(NO_PIXELFORMAT); //Not supported by libavg
                return;
            } else if (mode == MODE_320x240_YUV422) {
                pSize = IntPoint(320,240);
                pPixelFormat = PixelFormat(YCbCr422);
                return;
            } else if (mode == MODE_640x480_YUV411) {
                pSize = IntPoint(640,480);
                pPixelFormat = PixelFormat(YCbCr411);
                return;
            } else if (mode == MODE_640x480_YUV422) {
                pSize = IntPoint(640,480);
                pPixelFormat = PixelFormat(YCbCr422);
                return;
            } else if (mode == MODE_640x480_RGB) {
                pSize = IntPoint(640,480);
                pPixelFormat = PixelFormat(R8G8B8);
                return;
            } else if (mode == MODE_640x480_MONO) {
                pSize = IntPoint(640,480);
                pPixelFormat = PixelFormat(I8);
                return;
            } else if (mode == MODE_640x480_MONO16) {
                pSize = IntPoint(640,480);
                pPixelFormat = PixelFormat(I16);
                return;
            } else {
                AVG_ASSERT(false);
                return;
            }
            break;
        } case FORMAT_1: {
            if (mode == MODE_800x600_YUV422) {
                pSize = IntPoint(800,600);
                pPixelFormat = PixelFormat(YCbCr422);
                return;
            } else if (mode == MODE_800x600_RGB) {
                pSize = IntPoint(800,600);
                pPixelFormat = PixelFormat(R8G8B8);
                return;
            } else if (mode == MODE_800x600_MONO) {
                pSize = IntPoint(800,600);
                pPixelFormat = PixelFormat(I8);
                return;
            } else if (mode == MODE_1024x768_YUV422) {
                pSize = IntPoint(1024,768);
                pPixelFormat = PixelFormat(YCbCr422);
                return;
            } else if (mode == MODE_1024x768_RGB) {
                pSize = IntPoint(1024,768);
                pPixelFormat = PixelFormat(R8G8B8);
                return;
            } else if (mode == MODE_1024x768_MONO) {
                pSize = IntPoint(1024,768);
                pPixelFormat = PixelFormat(I8);
                return;
            } else if (mode == MODE_800x600_MONO16) {
                pSize = IntPoint(800,600);
                pPixelFormat = PixelFormat(I16);
                return;
            } else if (mode == MODE_1024x768_MONO16) {
                pSize = IntPoint(1024,768);
                pPixelFormat = PixelFormat(I16);
                return;
            } else {
                AVG_ASSERT(false);
                return;
            }
            break;
        } case FORMAT_2: {
            if (mode == MODE_1280x960_YUV422) {
                pSize = IntPoint(1280,960);
                pPixelFormat = PixelFormat(YCbCr422);
                return;
            } else if (mode == MODE_1280x960_RGB) {
                pSize = IntPoint(1280,960);
                pPixelFormat = PixelFormat(R8G8B8);
                return;
            } else if (mode == MODE_1280x960_MONO) {
                pSize = IntPoint(1280,960);
                pPixelFormat = PixelFormat(I8);
                return;
            } else if (mode == MODE_1600x1200_YUV422) {
                pSize = IntPoint(1600,1200);
                pPixelFormat = PixelFormat(YCbCr422);
                return;
            } else if (mode == MODE_1600x1200_RGB) {
                pSize = IntPoint(1600,1200);
                pPixelFormat = PixelFormat(R8G8B8);
                return;
            } else if (mode == MODE_1600x1200_MONO) {
                pSize = IntPoint(1600,1200);
                pPixelFormat = PixelFormat(I8);
                return;
            } else if (mode == MODE_1280x960_MONO16) {
                pSize = IntPoint(1280,960);
                pPixelFormat = PixelFormat(I16);
                return;
            } else if (mode == MODE_1600x1200_MONO16) {
                pSize = IntPoint(1600,1200);
                pPixelFormat = PixelFormat(I16);
                return;
            } else {
                AVG_ASSERT(false);
                return;
            }
            break;
        } default: {
            AVG_ASSERT(false);
            return;
        }
    }
}

}
