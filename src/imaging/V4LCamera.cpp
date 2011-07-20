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
//  V4L2/libavg compliance by 02L > Outside Standing Level

#include "V4LCamera.h"

#include "../base/ScopeTimer.h"
#include "../base/TimeSource.h"
#include "../base/Logger.h"
#include "../base/Exception.h"
#include "../base/StringHelper.h"

#include <sys/stat.h>
#include <sys/ioctl.h>
#include <sys/types.h>
#include <sys/mman.h>
#include <fcntl.h>
#include <errno.h>

#include <linux/videodev2.h>

#include <stdio.h>
#include <sys/time.h>
#include <unistd.h>
#include <iostream>
#include <cstring>

#define CLEAR(x) memset (&(x), 0, sizeof (x))

using namespace avg;
using namespace std;

// anonymous namespace holding private (C-static-like) functions
namespace {
    int xioctl(int fd, int request, void * arg)
    {
        int rc;

        do {
            rc = ioctl(fd, request, arg);
        } while (rc == -1 && EINTR == errno);

        return rc;
    }
}


V4LCamera::V4LCamera(std::string sDevice, int channel, IntPoint size, PixelFormat camPF,
        PixelFormat destPF, double frameRate)
    : Camera(camPF, destPF),
      m_Fd(-1),
      m_Channel(channel),
      m_sDevice(sDevice),
      m_ImgSize(size),
      m_FrameRate(frameRate)
{
    m_v4lPF = getV4LPF(camPF);
    if (m_sDevice == "") {
        m_sDevice = "/dev/video0";
    }
    if (m_Channel == -1) {
        m_Channel = 0;
    }

    m_FeaturesNames[V4L2_CID_BRIGHTNESS] = "brightness";
    m_FeaturesNames[V4L2_CID_CONTRAST] = "contrast";
    m_FeaturesNames[V4L2_CID_GAIN] = "gain";
    m_FeaturesNames[V4L2_CID_EXPOSURE] = "exposure";
    m_FeaturesNames[V4L2_CID_WHITENESS] = "whiteness";
    m_FeaturesNames[V4L2_CID_GAMMA] = "gamma";
    m_FeaturesNames[V4L2_CID_SATURATION] = "saturation";

    struct stat st;
    if (stat(m_sDevice.c_str(), &st) == -1) {
        fatalError(string("Unable to access v4l2 device '")+m_sDevice+"'." );
    }

    if (!S_ISCHR (st.st_mode)) {
        fatalError(string("'")+m_sDevice+" is not a v4l2 device.");
    }

    m_Fd = ::open(m_sDevice.c_str(), O_RDWR /* required */ | O_NONBLOCK, 0);

    if (m_Fd == -1) {
        fatalError(string("Unable to open v4l2 device '") + m_sDevice + "'.");
    }

    initDevice();
    AVG_TRACE(Logger::CONFIG, "V4L2 Camera opened");
}

V4LCamera::~V4LCamera()
{
    close();
}

void V4LCamera::close()
{
    enum v4l2_buf_type type = V4L2_BUF_TYPE_VIDEO_CAPTURE;
    int rc = xioctl(m_Fd, VIDIOC_STREAMOFF, &type);
    if (rc == -1) {
        AVG_TRACE(Logger::ERROR, "VIDIOC_STREAMOFF");
    }
    vector<Buffer>::iterator it;
    for (it = m_vBuffers.begin(); it != m_vBuffers.end(); ++it) {
        int err = munmap(it->start, it->length);
        AVG_ASSERT (err != -1);
    }
    m_vBuffers.clear();

    ::close(m_Fd);
    AVG_TRACE(Logger::CONFIG, "V4L2 Camera closed");

    m_Fd = -1;
}

IntPoint V4LCamera::getImgSize()
{
    return m_ImgSize;
}

int V4LCamera::getV4LPF(PixelFormat pf)
{
    switch (pf) {
        case I8:
            return V4L2_PIX_FMT_GREY;
        case YCbCr411:
            return V4L2_PIX_FMT_Y41P;
        case YCbCr422:
            return V4L2_PIX_FMT_UYVY;
        case YUYV422:
            return V4L2_PIX_FMT_YUYV;
        case YCbCr420p:
            return V4L2_PIX_FMT_YUV420;
        case R8G8B8:
            return V4L2_PIX_FMT_BGR24;
        default:
            throw Exception(AVG_ERR_INVALID_ARGS,
                    "Unsupported or illegal value for camera pixel format '"
                + getPixelFormatString(pf) + "'.");
    }
}

BitmapPtr V4LCamera::getImage(bool bWait)
{
    struct v4l2_buffer buf;
    CLEAR(buf);

    // wait for incoming data blocking, timeout 2s
    if (bWait) {
        fd_set fds;
        struct timeval tv;
        int rc;

        FD_ZERO(&fds);
        FD_SET(m_Fd, &fds);

        /* Timeout. */
        tv.tv_sec = 2;
        tv.tv_usec = 0;

        rc = select (m_Fd+1, &fds, NULL, NULL, &tv);

        // caught signal or something else
        if (rc == -1) {
            AVG_TRACE(Logger::WARNING, "V4L2: select failed.");
            return BitmapPtr();
        }
        // timeout
        if (rc == 0) {
            AVG_TRACE(Logger::WARNING, "V4L2: Timeout while waiting for image data");
            return BitmapPtr();
        }
    }

    buf.type = V4L2_BUF_TYPE_VIDEO_CAPTURE;
    buf.memory = V4L2_MEMORY_MMAP;

    // dequeue filled buffer
    if (xioctl (m_Fd, VIDIOC_DQBUF, &buf) == -1) {
        if (errno == EAGAIN) {
            return BitmapPtr();
        } else {
            cerr << strerror(errno) << endl;
            AVG_ASSERT(false);
        }
    }

    unsigned char * pCaptureBuffer = (unsigned char*)m_vBuffers[buf.index].start;

    double lineLen;
    switch (getCamPF()) {
        case YCbCr411:
            lineLen = m_ImgSize.x*1.5;
            break;
        case YCbCr420p:
            lineLen = m_ImgSize.x;
            break;
        default:
            lineLen = m_ImgSize.x*Bitmap::getBytesPerPixel(getCamPF());
    }
    BitmapPtr pCamBmp(new Bitmap(m_ImgSize, getCamPF(), pCaptureBuffer, lineLen,
            false, "TempCameraBmp"));

    BitmapPtr pDestBmp = convertCamFrameToDestPF(pCamBmp);

    // enqueues free buffer for mmap
    if (-1 == xioctl (m_Fd, VIDIOC_QBUF, &buf)) {
        throw(Exception(AVG_ERR_CAMERA_FATAL,
                "V4L Camera: failed to enqueue image buffer."));
    }

    return pDestBmp;
}

bool V4LCamera::isCameraAvailable()
{
    return m_bCameraAvailable;
}

const std::string& V4LCamera::getDevice() const
{
    return m_sDevice;
}

const std::string& V4LCamera::getDriverName() const
{
    return m_sDriverName;
}

double V4LCamera::getFrameRate() const
{
    return m_FrameRate;
}

std::string V4LCamera::getFeatureName(V4LCID_t v4lFeature)
{
    std::string sName = m_FeaturesNames[v4lFeature];
    if (sName == "") {
        sName = "UNKNOWN";
    }

    return sName;
}

V4LCID_t V4LCamera::getFeatureID(CameraFeature feature) const
{
    V4LCID_t v4lFeature;
    if (feature == CAM_FEATURE_BRIGHTNESS) {
        v4lFeature = V4L2_CID_BRIGHTNESS;
    } else if (feature == CAM_FEATURE_CONTRAST) {
        v4lFeature = V4L2_CID_CONTRAST;
    }  else if (feature == CAM_FEATURE_GAIN) {
        v4lFeature = V4L2_CID_GAIN;
    } else if (feature == CAM_FEATURE_EXPOSURE) {
        v4lFeature = V4L2_CID_EXPOSURE;
    } else if (feature == CAM_FEATURE_GAMMA) {
        v4lFeature = V4L2_CID_GAMMA;
    } else if (feature == CAM_FEATURE_SATURATION) {
        v4lFeature = V4L2_CID_SATURATION;
    } else {
        AVG_TRACE(Logger::WARNING, "feature " << cameraFeatureToString(feature)
                << " not supported for V4L2.");
        return -1;
    }

    return v4lFeature;
}

bool V4LCamera::isFeatureSupported(V4LCID_t v4lFeature) const
{
    struct v4l2_queryctrl queryCtrl;

    CLEAR(queryCtrl);
    queryCtrl.id = v4lFeature;

    if (ioctl (m_Fd, VIDIOC_QUERYCTRL, &queryCtrl) == -1) {
        if (errno != EINVAL) {
            cerr << "Got " << strerror(errno) << endl;
            AVG_ASSERT(false);
            return false;
        } else {
            return false;
        }
    } else if (queryCtrl.flags & V4L2_CTRL_FLAG_DISABLED) {
        return false;
    } else {
        return true;
    }
}

int V4LCamera::getFeature(CameraFeature feature) const
{
    V4LCID_t v4lFeature = getFeatureID(feature);

    FeatureMap::const_iterator it = m_Features.find(v4lFeature);

    if (it == m_Features.end()) {
        return 0;
    } else {
        return it->second;
    }
}

void V4LCamera::setFeature(V4LCID_t v4lFeature, int value)
{
    if (!m_bCameraAvailable) {
        AVG_TRACE(Logger::WARNING, "setFeature() called before opening device: ignored");
        return;
    }

    if (!isFeatureSupported(v4lFeature)) {
        AVG_TRACE(Logger::WARNING, "Camera feature " << getFeatureName(v4lFeature) <<
            " is not supported by hardware");
        return;
    }

    struct v4l2_control control;

    CLEAR(control);
    control.id = v4lFeature;
    control.value = value;

//    AVG_TRACE(Logger::APP, "Setting feature " << getFeatureName(v4lFeature) <<
//      " to "<< value);

    if (ioctl(m_Fd, VIDIOC_S_CTRL, &control) == -1) {
        AVG_TRACE(Logger::ERROR, "Cannot set feature " <<
            m_FeaturesNames[v4lFeature]);
    }
}

void V4LCamera::setFeatureOneShot(CameraFeature feature)
{
    AVG_TRACE(Logger::WARNING, "setFeatureOneShot is not supported by V4L cameras.");
}

int V4LCamera::getWhitebalanceU() const
{
    AVG_TRACE(Logger::WARNING, "getWhitebalance is not supported by V4L cameras.");
    return 0;
}

int V4LCamera::getWhitebalanceV() const
{
    AVG_TRACE(Logger::WARNING, "getWhitebalance is not supported by V4L cameras.");
    return 0;
}

void V4LCamera::setWhitebalance(int u, int v, bool bIgnoreOldValue)
{
    setFeature(V4L2_CID_RED_BALANCE, u);
    setFeature(V4L2_CID_BLUE_BALANCE, v);
}

int V4LCamera::dumpCameras_open(int j)
{
    stringstream minorDeviceNumber;
    minorDeviceNumber << j;
    string address = "/dev/video";
    string result = address + minorDeviceNumber.str();
    int fd = ::open(result.c_str(), O_RDWR /* required */ | O_NONBLOCK, 0);
    return fd;
}

v4l2_capability dumpCameraCapabilities(int fd)
{
    v4l2_capability capability;
    memset(&capability, 0, sizeof(capability));
    int rc = ioctl(fd, VIDIOC_QUERYCAP, &capability);
    if (rc != -1) {
        cout << capability.card << ":" << endl;
        cout << "    Driver:  " << capability.driver << endl;
        cout << "  Location:  " << capability.bus_info;
        cout << endl << endl;
    }
    return capability;
}

v4l2_capability getCamCapabilities(int fd)
{
    v4l2_capability capability;
    memset(&capability, 0, sizeof(capability));
    int rc = ioctl(fd, VIDIOC_QUERYCAP, &capability);
    return capability;
}

void V4LCamera::dumpSupportedImgFormats(int fd)
{
    cout << "Suported Image Formats:" << endl;
    for (int i = 0;; i++) {
        v4l2_fmtdesc fmtDesc;
        memset(&fmtDesc, 0, sizeof(fmtDesc));
        fmtDesc.index = i;
        fmtDesc.type  = V4L2_BUF_TYPE_VIDEO_CAPTURE;
        int rc = ioctl(fd, VIDIOC_ENUM_FMT, &fmtDesc);
        if (rc == -1) {
            break;
        }
        v4l2_frmsizeenum frmSizeEnum;
        memset(&frmSizeEnum, 0, sizeof (frmSizeEnum));
        frmSizeEnum.index = 0;
        frmSizeEnum.pixel_format = fmtDesc.pixelformat;
        bool bSupported = false;
        while (ioctl(fd, VIDIOC_ENUM_FRAMESIZES, &frmSizeEnum) == 0) {
            const char * sAvgPixelformat = pixelformatToString(fmtDesc.pixelformat);
            if (sAvgPixelformat != "NOTAVGSUPPORTED") {
                v4l2_frmivalenum frmIvalEnum;
                cout << "   " << sAvgPixelformat << " ";
                cout << "  (" << frmSizeEnum.discrete.width << ", ";
                cout << frmSizeEnum.discrete.height << ")";
                cout << "   fps: ";
                memset (&frmIvalEnum, 0, sizeof (frmIvalEnum));
                frmIvalEnum.index = 0;
                frmIvalEnum.pixel_format = frmSizeEnum.pixel_format;
                frmIvalEnum.width = frmSizeEnum.discrete.width;
                frmIvalEnum.height = frmSizeEnum.discrete.height;
                while (ioctl(fd, VIDIOC_ENUM_FRAMEINTERVALS, &frmIvalEnum) == 0) {
                    cout << frmIvalEnum.discrete.denominator << "/";
                    frmIvalEnum.index++;
                }
                cout << endl;
            }
            frmSizeEnum.index++;
        }
    }
}

void V4LCamera::dumpCameraControls(int fd)
{
    cout << endl << "Camera Controls:" << endl;
    v4l2_queryctrl queryCtrl;
    for (queryCtrl.id = V4L2_CID_BASE; queryCtrl.id < V4L2_CID_LASTP1; queryCtrl.id++) {
        int rc = ioctl (fd, VIDIOC_QUERYCTRL, &queryCtrl);
        if (rc != -1) {
            if (queryCtrl.flags & V4L2_CTRL_FLAG_DISABLED) {
                continue;
            }
            cout << "  " << queryCtrl.name << ":" << endl;
            cout << "    Min: " << queryCtrl.minimum << " | ";
            cout << "Max: " << queryCtrl.maximum << " | ";
            cout << "Default: "<< queryCtrl.default_value << endl;
        } else {
            if (errno != EINVAL) {
                perror("VIDIOC_QUERYCTRL");
                exit(EXIT_FAILURE);
            }
        }
    }
}

void V4LCamera::dumpCameras()
{
    for(int j = 0; j < 256; j++){
        int fd = dumpCameras_open(j);
        if (fd != -1) {
            cout << "------------------------Video4linux Camera-------------------------";
            cout << endl;
            cout << "/dev/video" << j << " ";
            v4l2_capability capability = dumpCameraCapabilities(fd);
            if (capability.capabilities & V4L2_CAP_VIDEO_CAPTURE) {
                dumpSupportedImgFormats(fd);
                dumpCameraControls(fd);
            }
            cout << "-------------------------------------------------------------------";
            cout << endl;
        }
    }
}

const char * V4LCamera::pixelformatToString(unsigned int pixelformat)
{
    const char * sAvgPixelformat;
    switch (pixelformat) {
        case v4l2_fourcc('Y','U','Y','V'):
            sAvgPixelformat = "YUYV422";
            break;
        case v4l2_fourcc('U','Y','V','Y'):
            sAvgPixelformat = "YUV422";
            break;
        case v4l2_fourcc('G','R','E','Y'):
            sAvgPixelformat = "I8";
            break;
        case v4l2_fourcc('Y','1','6',' '):
            sAvgPixelformat = "I16";
            break;
        case v4l2_fourcc('R','G','B','3'):
            sAvgPixelformat = "RGB";
            break;
        case v4l2_fourcc('B','G','R','3'):
            sAvgPixelformat = "BGR";
            break;
        default:
            sAvgPixelformat = "NOTAVGSUPPORTED";
            break;
    }
    return sAvgPixelformat;
}

int V4LCamera::countCameras(){
    int numberOfCameras = 0;
    for(int j = 0; j < 256; j++){
        int fd = dumpCameras_open(j);
        if (fd != -1) {
            numberOfCameras++;
        }
    }
    //cout<< "countCameras() number of cameras: "<< numberOfCameras << endl;
    return numberOfCameras;
}

CameraInfo V4LCamera::listCameraInfo(int deviceNumber)
{
    int fd = dumpCameras_open(deviceNumber);
    cout<<"fd di listCamerainfo(): "<<fd<<endl;
    if (fd != -1){
        CameraInfo camInfo;
        camInfo.name = getCamName(deviceNumber);
        CamName it = camInfo.name;
        cout << "Camera Driver: " << it.driver << endl;
        cout << "Camera Device: " << it.device << endl;
        cout << "Camera Unit:   " << it.unit   << endl;
        cout << "Camera fw800: " << it.fw800  << endl;
        v4l2_capability capability = getCamCapabilities(fd);
        if (capability.capabilities & V4L2_CAP_VIDEO_CAPTURE) {
            getCamImgFormats(fd, std::back_inserter(camInfo.lImgFormats));
            getCamControls(fd, std::back_inserter(camInfo.lCamControls));
            //for debug
            for (std::list<CamImageFormat>::iterator it = camInfo.lImgFormats.begin(); it != camInfo.lImgFormats.end(); it++){
            cout<<"width: "<< (*it).iWidth << endl;
            cout<<"height: "<< (*it).iHeight << endl;
            cout<<"PixelFormat: "<< (*it).sPixelformat << endl;
            cout<<"Framerate: "<< (*it).dFramerate << endl;
            }
            //for debug
            for (std::list<CamControls>::iterator it = camInfo.lCamControls.begin(); it != camInfo.lCamControls.end(); it++){
                cout << "Control Name: "<< (*it).sControlName << endl;
                cout << "Control Min:  "<< (*it).iMin << endl;
                cout << "Control Max:  "<< (*it).iMax << endl;
                cout << "Control Default:  "<< (*it).iDefault << endl;
            }
            return camInfo;
        }
    }
}

CamName V4LCamera::getCamName(int fd) {
    CamName name;
    cout<< "fd=" << fd;
    name.driver = "video4linux";
    std::stringstream ss;
    ss << fd;
    name.device = "/dev/video";
    name.device += ss.str();
    name.unit = -1;//TODO: is this always -1 ???????
    name.fw800 = 0;
    //for debug
    cout << "getcameraname() device:" << name.device << endl;
    return name;
}


typedef std::back_insert_iterator<std::list<CamImageFormat> > OutputIterFormats;
void V4LCamera::getCamImgFormats(int fd, OutputIterFormats out) {    
    for (int i = 0;; i++) {
        v4l2_fmtdesc fmtDesc;
        memset(&fmtDesc, 0, sizeof(fmtDesc));
        fmtDesc.index = i;
        fmtDesc.type  = V4L2_BUF_TYPE_VIDEO_CAPTURE;
        int rc = ioctl(fd, VIDIOC_ENUM_FMT, &fmtDesc);
        if (rc == -1) {
            break;
        }
        v4l2_frmsizeenum frmSizeEnum;
        memset(&frmSizeEnum, 0, sizeof (frmSizeEnum));
        frmSizeEnum.index = 0;
        frmSizeEnum.pixel_format = fmtDesc.pixelformat;
        bool bSupported = false;
        while (ioctl(fd, VIDIOC_ENUM_FRAMESIZES, &frmSizeEnum) == 0) {
            std::string sAvgPixelformat = pixelformatToString(fmtDesc.pixelformat);
            if (sAvgPixelformat != "NOTAVGSUPPORTED") {
                v4l2_frmivalenum frmIvalEnum;
                memset (&frmIvalEnum, 0, sizeof (frmIvalEnum));
                frmIvalEnum.index = 0;
                frmIvalEnum.pixel_format = frmSizeEnum.pixel_format;
                frmIvalEnum.width = frmSizeEnum.discrete.width;
                frmIvalEnum.height = frmSizeEnum.discrete.height;
                while (ioctl(fd, VIDIOC_ENUM_FRAMEINTERVALS, &frmIvalEnum) == 0) {
                    CamImageFormat camImFormat;  
                    camImFormat.iWidth = frmSizeEnum.discrete.width;
                    camImFormat.iHeight = frmSizeEnum.discrete.height;
                    camImFormat.sPixelformat = sAvgPixelformat;
                    camImFormat.dFramerate = frmIvalEnum.discrete.denominator;
                    //dereferencing iterator for assignment.
                    *(out++) = camImFormat;
                    frmIvalEnum.index++;
                }
            }
            frmSizeEnum.index++;
        }
    }
}

string V4LtoAVG_ctrlNames(__u8* name)
{
    //TODO:some controls are not support by libavg, the names should be masked;
    std::stringstream ss;
    ss << name;
    string sControlName = ss.str() ;
    return sControlName;
}

typedef std::back_insert_iterator<std::list<CamControls> > OutputIterCtrls;
void V4LCamera::getCamControls(int fd, OutputIterCtrls out)
{
    v4l2_queryctrl queryCtrl;
    //std::list<CamControls> lCamControls;
    for (queryCtrl.id = V4L2_CID_BASE; queryCtrl.id < V4L2_CID_LASTP1; queryCtrl.id++) {
        int rc = ioctl (fd, VIDIOC_QUERYCTRL, &queryCtrl);
        if (rc != -1) {
            if (queryCtrl.flags & V4L2_CTRL_FLAG_DISABLED) {
                continue;
            }
            CamControls camControls;
            camControls.sControlName =V4LtoAVG_ctrlNames(queryCtrl.name); //TODO:V4LtoAVG_ctrl..
            camControls.iMin = queryCtrl.minimum;
            camControls.iMax = queryCtrl.maximum;
            camControls.iDefault = queryCtrl.default_value;
            //dereferencing iterator for assignment.
            *(out++) = camControls;
        } else {
            if (errno != EINVAL) {
                perror("VIDIOC_QUERYCTRL");
                exit(EXIT_FAILURE);
            }
        }
    }
}




void V4LCamera::setFeature(CameraFeature feature, int value, bool bIgnoreOldValue)
{
    // ignore -1 coming from default unbiased cameranode parameters
    if (value < 0) {
        return;
    }

    V4LCID_t v4lFeature = getFeatureID(feature);
    m_Features[v4lFeature] = value;

//    AVG_TRACE(Logger::WARNING,"Setting feature " << sFeature <<
//        " to " << value);
    if (m_bCameraAvailable) {
        setFeature(v4lFeature, value);
    }
}

void V4LCamera::startCapture()
{
//  AVG_TRACE(Logger::APP, "Entering startCapture()...");

    unsigned int i;
    enum v4l2_buf_type type;

    for (i = 0; i < m_vBuffers.size(); ++i) {
        struct v4l2_buffer buf;

        CLEAR(buf);

        buf.type = V4L2_BUF_TYPE_VIDEO_CAPTURE;
        buf.memory = V4L2_MEMORY_MMAP;
        buf.index = i;

        int err = xioctl(m_Fd, VIDIOC_QBUF, &buf);
        AVG_ASSERT(err != -1);
    }

    type = V4L2_BUF_TYPE_VIDEO_CAPTURE;
    int err= xioctl (m_Fd, VIDIOC_STREAMON, &type);
    AVG_ASSERT(err != -1);
}

void V4LCamera::initDevice()
{
//  AVG_TRACE(Logger::APP, "Entering initDevice()...");

    struct v4l2_capability cap;
    struct v4l2_cropcap CropCap;
    struct v4l2_crop Crop;
    struct v4l2_format fmt;
    struct v4l2_streamparm StreamParam;

    if (xioctl(m_Fd, VIDIOC_QUERYCAP, &cap) == -1) {
        close();
        fatalError(m_sDevice + " is not a valid V4L2 device.");
    }

    if (!(cap.capabilities & V4L2_CAP_VIDEO_CAPTURE)) {
        close();
        fatalError(m_sDevice + " does not support capturing");
    }

    if (!(cap.capabilities & V4L2_CAP_STREAMING)) {
        close();
        fatalError(m_sDevice + " does not support streaming i/os");
    }
    m_sDriverName = (const char *)cap.driver;

    /* Select video input, video standard and tune here. */
    CLEAR(CropCap);
    CropCap.type = V4L2_BUF_TYPE_VIDEO_CAPTURE;

    if (xioctl(m_Fd, VIDIOC_CROPCAP, &CropCap) == 0) {
        Crop.type = V4L2_BUF_TYPE_VIDEO_CAPTURE;
        Crop.c = CropCap.defrect; /* reset to default */

        if (-1 == xioctl(m_Fd, VIDIOC_S_CROP, &Crop)) {
            switch (errno) {
                case EINVAL:
                    /* Cropping not supported. */
                    break;
                default:
                    /* Errors ignored. */
                    break;
            }
        }
    } else {
            /* Errors ignored. */
    }

    CLEAR(fmt);

    fmt.type = V4L2_BUF_TYPE_VIDEO_CAPTURE;
    fmt.fmt.pix.width = m_ImgSize.x;
    fmt.fmt.pix.height = m_ImgSize.y;
    fmt.fmt.pix.pixelformat = m_v4lPF;
    fmt.fmt.pix.field = V4L2_FIELD_ANY;
    int rc = xioctl(m_Fd, VIDIOC_S_FMT, &fmt);
    if (fmt.fmt.pix.width != m_ImgSize.x || fmt.fmt.pix.height != m_ImgSize.y || rc == -1)
    {
        throw(Exception(AVG_ERR_CAMERA_NONFATAL,
                string("Unable to set V4L camera image format: '")
                +strerror(errno)
                +"'. Try using avg_showcamera.py --dump to find out what\
                        the device supports."));
    }

    CLEAR(StreamParam);

    StreamParam.type = V4L2_BUF_TYPE_VIDEO_CAPTURE;
    StreamParam.parm.capture.timeperframe.numerator = 1;
    StreamParam.parm.capture.timeperframe.denominator = (int) m_FrameRate;
    rc = xioctl(m_Fd, VIDIOC_S_PARM, &StreamParam);
    if (m_FrameRate != StreamParam.parm.capture.timeperframe.denominator || rc == -1) {
        throw(Exception(AVG_ERR_CAMERA_NONFATAL,
                string("Unable to set V4L camera framerate: '")
                +strerror(errno)
                +"'. Try using avg_showcamera.py --dump to find out what\
                        the device supports."));
    }
    m_FrameRate = (double)StreamParam.parm.capture.timeperframe.denominator / \
            (double)StreamParam.parm.capture.timeperframe.numerator;

    initMMap ();

    // TODO: string channel instead of numeric
    // select channel
    if (xioctl(m_Fd, VIDIOC_S_INPUT, &m_Channel) == -1) {
        close();
        fatalError(string("Cannot set MUX channel ")+toString(m_Channel));
    }

    m_bCameraAvailable = true;

    for (FeatureMap::iterator it=m_Features.begin(); it != m_Features.end(); it++) {
        setFeature(it->first, it->second);
    }


}

void V4LCamera::initMMap()
{
    struct v4l2_requestbuffers req;
    CLEAR(req);

    req.count = 4;
    req.type = V4L2_BUF_TYPE_VIDEO_CAPTURE;
    req.memory = V4L2_MEMORY_MMAP;

    if (xioctl(m_Fd, VIDIOC_REQBUFS, &req) == -1) {
        if (EINVAL == errno) {
            close();
            fatalError(m_sDevice+" does not support memory mapping");
        } else {
            cerr << "errno: " << strerror(errno);
            AVG_ASSERT(false);
        }
    }

    if (req.count < 2) {
        cerr << "Insufficient buffer memory on " << m_sDevice;
        AVG_ASSERT(false);
    }

    m_vBuffers.clear();

    for (int i = 0; i < int(req.count); ++i) {
        Buffer tmp;
        struct v4l2_buffer buf;

        CLEAR (buf);

        buf.type = V4L2_BUF_TYPE_VIDEO_CAPTURE;
        buf.memory = V4L2_MEMORY_MMAP;
        buf.index = i;

        if (xioctl(m_Fd, VIDIOC_QUERYBUF, &buf) == -1) {
            AVG_ASSERT(false);
        }

        tmp.length = buf.length;

        tmp.start = mmap (NULL /* start anywhere */,
            buf.length,
            PROT_READ | PROT_WRITE /* required */,
            MAP_SHARED /* recommended */,
            m_Fd, buf.m.offset);

        if (MAP_FAILED == tmp.start) {
            AVG_ASSERT(false);
        }

        m_vBuffers.push_back(tmp);
    }
}



