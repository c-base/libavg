
#include "V4LCamera.h"

#include "../base/ScopeTimer.h"
#include "../base/TimeSource.h"
#include "../base/Logger.h"

#include <sys/stat.h>
#include <sys/ioctl.h>
#include <sys/types.h>
#include <sys/mman.h>
#include <fcntl.h>
#include <errno.h>
#include <linux/videodev2.h>

       
#define CLEAR(x) memset (&(x), 0, sizeof (x))

using namespace avg;

// anonymous namespace holding private (C-static-like) functions
namespace {
	int xioctl (int fd, int request, void * arg)
	{
        int r;

        do r = ioctl (fd, request, arg);
        while (r == -1 && EINTR == errno);

        return r;
	}
}

V4LCamera::V4LCamera(std::string sDevice, int Channel, std::string sMode, 
	bool bColor)
	: fd_(-1), Channel_(Channel), m_sDevice(sDevice), 
	  ioMethod_(V4LCamera::IO_METHOD_MMAP),
	  m_sMode(),
	  m_bCameraAvailable(false)
    /*: 
      m_FrameRate(FrameRate),
      m_bColor(bColor),
      m_bCameraAvailable(false) */
{
//	ioMethod_ = IO_METHOD_READ;
}

V4LCamera::~V4LCamera() 
{}

void V4LCamera::open()
{
	struct stat st; 

    if ( stat(m_sDevice.c_str(), &st) == -1) {
    	AVG_TRACE(Logger::ERROR, "Unable to open v4l device " << m_sDevice);
        // TODO: Disable camera instead of exit(-1).
        exit(-1);
    }

    if (!S_ISCHR (st.st_mode)) {
    	AVG_TRACE(Logger::ERROR, m_sDevice + " is not a v4l device");
        // TODO: Disable camera instead of exit(-1).
        exit(-1);
    }

    fd_ = ::open(m_sDevice.c_str(), O_RDWR /* required */ | O_NONBLOCK, 0);

    if (fd_ == -1) {
    	AVG_TRACE(Logger::ERROR, "Unable to open v4l device " << m_sDevice);
        // TODO: Disable camera instead of exit(-1).
        exit(-1);
    }
    
    AVG_TRACE(Logger::APP, "Device opened, calling initDevice()...");
    initDevice();
    startCapture();
}

void V4LCamera::close()
{
	AVG_TRACE(Logger::APP, "Closing Camera...");
	if ( ::close(fd_) == -1)
        AVG_TRACE(Logger::ERROR, "Error on closing v4l device");

    fd_ = -1;
    m_bCameraAvailable = false;
}

IntPoint V4LCamera::getImgSize()
{
	return IntPoint(640, 480);
}

static ProfilingZone CameraConvertProfilingZone("      Camera format conversion");

BitmapPtr V4LCamera::getImage(bool /*bWait*/)
{
//	AVG_TRACE(Logger::APP, "getImage()");
	unsigned char *pSrc = 0;
	
	struct v4l2_buffer buf;
	CLEAR(buf);
	
	switch (ioMethod_)
	{
	case IO_METHOD_READ:
		if (read (fd_, buffers_[0].start, buffers_[0].length) == -1) {
	//		AVG_TRACE(Logger::WARNING, "Frame not ready");
			return BitmapPtr();
		}
	//	else AVG_TRACE(Logger::WARNING, "Frame read");

		pSrc = (unsigned char *)(buffers_[0].start);

		break;

	case IO_METHOD_MMAP:
		
		buf.type = V4L2_BUF_TYPE_VIDEO_CAPTURE;
		buf.memory = V4L2_MEMORY_MMAP;
		
		if (-1 == xioctl (fd_, VIDIOC_DQBUF, &buf))
		{
            switch (errno)
            {
	            case EAGAIN:
	                    return BitmapPtr();
	
	            case EIO:
						AVG_TRACE(Logger::ERROR, "EIO");
						exit(-1);
						break;

	            case EINVAL:
						AVG_TRACE(Logger::ERROR, "EINVAL");
						exit(-1);
						break;

	            default:
	                    AVG_TRACE(Logger::ERROR, "VIDIOC_DQBUF");
	                    exit(-1);
            }
	    }
	    
	    pSrc = (unsigned char*)buffers_[buf.index].start;
	    
//	    AVG_TRACE(Logger::APP,"mmap() dequeueing buffer index=" << buf.index);
		break;
	}

	IntPoint size = getImgSize();

// I8 is not suitable for CameraNode
//	BitmapPtr pCurBitmap = BitmapPtr(new Bitmap(size, I8));
//	Bitmap TempBmp(size, I8, (unsigned char *)buf.start,
//    pCurBitmap->copyPixels(TempBmp);

	BitmapPtr pCurBitmap = BitmapPtr(new Bitmap(size, B8G8R8X8));

	//cerr << pCurBitmap->dump();
	//return pCurBitmap;

/*	
	// BGR32 sets alpha channel to 0x00	
	char *bImage = (char *)buf.start;
	for (int ip = 3 ; ip < buf.length ; ip+=4) bImage[ip] = 0xFF;

	// This way leads to an incorrect assignment of stripes	
	BitmapPtr bmImg = BitmapPtr( new Bitmap(size, B8G8R8X8, (unsigned char *)buf.start,
		size.x, false, "TempCameraBmp" ));

	bmImg->save("dumpimg.bmp");
	
	return bmImg;
*/

 
 	ScopeTimer Timer(CameraConvertProfilingZone);

	// BGR32 / B8G8R8X8 	
    unsigned char *pDest = pCurBitmap->getPixels();
	for (int imgp = 0 ; imgp < size.x * size.y ; ++imgp)
	{
		*pDest++ = *pSrc++; // Blue
		*pDest++ = *pSrc++; // Green
		*pDest++ = *pSrc++; // Red
		*pDest++ = 0xFF;	// Alpha
		pSrc++;		 		// Discard Alpha channel on src (usually 0x0)
	}

	// free a buffer for mmap
	if (ioMethod_ == IO_METHOD_MMAP)
	{
		if (-1 == xioctl (fd_, VIDIOC_QBUF, &buf))
		{
            AVG_TRACE(Logger::ERROR, "VIDIOC_DQBUF");
            exit(-1);
		}
	}
	
    return pCurBitmap;
}

bool V4LCamera::isCameraAvailable()
{
    return true;
}

const std::string& V4LCamera::getDevice() const
{
    return m_sDevice;
}

double V4LCamera::getFrameRate() const
{
    return 25;
}

const std::string& V4LCamera::getMode() const
{
    return m_sMode;
}


unsigned int V4LCamera::getFeature(const std::string& sFeature) const
{
    return 0;
}

void V4LCamera::setFeature(const std::string& sFeature, int Value)
{
	AVG_TRACE(Logger::APP, "Setting feature " << sFeature << " value " << Value);
}

void V4LCamera::startCapture()
{
	AVG_TRACE(Logger::APP, "Entering startCapture()...");

	unsigned int i;
	enum v4l2_buf_type type;

	switch (ioMethod_) {
	case IO_METHOD_READ:
		/* Nothing to do. */
		break;

	case IO_METHOD_MMAP:
		for (i = 0; i < buffers_.size(); ++i) {
            struct v4l2_buffer buf;

    		CLEAR (buf);

    		buf.type        = V4L2_BUF_TYPE_VIDEO_CAPTURE;
    		buf.memory      = V4L2_MEMORY_MMAP;
    		buf.index       = i;

    		if (-1 == xioctl (fd_, VIDIOC_QBUF, &buf))
    		{
	        	AVG_TRACE(Logger::ERROR, "VIDIOC_QBUF");
	            exit (-1);
    		}
		}
		
		type = V4L2_BUF_TYPE_VIDEO_CAPTURE;

		if (-1 == xioctl (fd_, VIDIOC_STREAMON, &type))
		{
        	AVG_TRACE(Logger::ERROR, "VIDIOC_STREAMON");
            exit (-1);
		}

		break;

	case IO_METHOD_USERPTR:
		for (i = 0; i < buffers_.size(); ++i) {
            		struct v4l2_buffer buf;

        		CLEAR (buf);

        		buf.type        = V4L2_BUF_TYPE_VIDEO_CAPTURE;
        		buf.memory      = V4L2_MEMORY_USERPTR;
			buf.index       = i;
			buf.m.userptr	= (unsigned long) buffers_[i].start;
			buf.length      = buffers_[i].length;

			if (-1 == xioctl (fd_, VIDIOC_QBUF, &buf))
			{
	        	AVG_TRACE(Logger::ERROR, "VIDIOC_QBUF");
	            exit (-1);
			}
		}

		type = V4L2_BUF_TYPE_VIDEO_CAPTURE;

		if (-1 == xioctl (fd_, VIDIOC_STREAMON, &type))
		{
        	AVG_TRACE(Logger::ERROR, "VIDIOC_STREAMON");
            exit (-1);
		}

		break;
	}
}

void V4LCamera::initDevice()
{
	AVG_TRACE(Logger::APP, "Entering initDevice()...");
	
    struct v4l2_capability cap;
    struct v4l2_cropcap cropcap;
    struct v4l2_crop crop;
    struct v4l2_format fmt;
	unsigned int min;

    if (-1 == xioctl (fd_, VIDIOC_QUERYCAP, &cap)) {
        if (EINVAL == errno) {
        	AVG_TRACE(Logger::ERROR, m_sDevice << " is no V4L2 device");
            exit (-1);
        } else {
        	AVG_TRACE(Logger::ERROR, "VIDIOC_QUERYCAP error");
            exit(-1);
        }
    }

    if (!(cap.capabilities & V4L2_CAP_VIDEO_CAPTURE)) {
        AVG_TRACE(Logger::ERROR, m_sDevice << " is no V4L2 device");
        exit(-1);
    }

	switch (ioMethod_) {
	case IO_METHOD_READ:
		if (!(cap.capabilities & V4L2_CAP_READWRITE)) {
			AVG_TRACE(Logger::ERROR, m_sDevice << " does not support read i/o");
			exit (-1);
		}

		break;

	case IO_METHOD_MMAP:
	case IO_METHOD_USERPTR:
		if (!(cap.capabilities & V4L2_CAP_STREAMING)) {
			AVG_TRACE(Logger::ERROR, m_sDevice << " does not support streaming i/o");
			exit (-1);
		}

		break;
	}


    /* Select video input, video standard and tune here. */
	CLEAR (cropcap);
    cropcap.type = V4L2_BUF_TYPE_VIDEO_CAPTURE;

    if (xioctl(fd_, VIDIOC_CROPCAP, &cropcap) == 0) {
        crop.type = V4L2_BUF_TYPE_VIDEO_CAPTURE;
        crop.c = cropcap.defrect; /* reset to default */

        if (-1 == xioctl (fd_, VIDIOC_S_CROP, &crop)) {
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

    CLEAR (fmt);

    fmt.type                = V4L2_BUF_TYPE_VIDEO_CAPTURE;
    fmt.fmt.pix.width       = getImgSize().x; 
    fmt.fmt.pix.height      = getImgSize().y;
//	fmt.fmt.pix.pixelformat = V4L2_PIX_FMT_GREY;
    fmt.fmt.pix.pixelformat = V4L2_PIX_FMT_BGR32;
    fmt.fmt.pix.field       = V4L2_FIELD_INTERLACED;

    if (xioctl(fd_, VIDIOC_S_FMT, &fmt) == -1)
        exit (-1);

	/* Note VIDIOC_S_FMT may change width and height. */

	/* Buggy driver paranoia. */
	min = fmt.fmt.pix.width * 2;
	if (fmt.fmt.pix.bytesperline < min)
		fmt.fmt.pix.bytesperline = min;
	min = fmt.fmt.pix.bytesperline * fmt.fmt.pix.height;
	if (fmt.fmt.pix.sizeimage < min)
		fmt.fmt.pix.sizeimage = min;

	switch (ioMethod_) {
	case IO_METHOD_READ:
		init_read (fmt.fmt.pix.sizeimage);
		break;

	case IO_METHOD_MMAP:
		init_mmap ();
		break;

	case IO_METHOD_USERPTR:
		init_userp (fmt.fmt.pix.sizeimage);
		break;
	}
	
	// select channel
	AVG_TRACE(Logger::APP, "Setting channel " << Channel_);
	if (xioctl(fd_, VIDIOC_S_INPUT, &Channel_) == -1) {
		exit(-1);
	}
	
	AVG_TRACE(Logger::APP, "initDevice() completed");
	
	m_bCameraAvailable = true;
}

void V4LCamera::init_read (unsigned int buffer_size)
{
	AVG_TRACE(Logger::APP, "entering init_read()...");
    buffers_.clear();
    Buffer tmp;

	tmp.length = buffer_size;
	tmp.start = malloc(buffer_size);

	if (!tmp.start) {
		fprintf (stderr, "Out of memory\n");
    	exit (-1);
	}
	
	buffers_.push_back(tmp);
	AVG_TRACE(Logger::APP, "init_read() completed");
}

void V4LCamera::init_mmap()
{
	struct v4l2_requestbuffers req;
    CLEAR (req);

    req.count = 4;
    req.type = V4L2_BUF_TYPE_VIDEO_CAPTURE;
    req.memory = V4L2_MEMORY_MMAP;

	if (xioctl (fd_, VIDIOC_REQBUFS, &req) == -1) {
        if (EINVAL == errno) {
            fprintf (stderr, "%s does not support memory mapping\n", 
            	m_sDevice.c_str());
            //exit (EXIT_FAILURE);
        } else {
            //errno_exit ("VIDIOC_REQBUFS");
        }
    }
    
    AVG_TRACE(Logger::APP, "init_mmap(): " << req.count << " buffers requested");

    if (req.count < 2) {
        fprintf (stderr, "Insufficient buffer memory on %s\n", 
        	m_sDevice.c_str());
        //exit (EXIT_FAILURE);
    }

    buffers_.clear();

    for (int i=0; i < req.count; ++i) {
    	Buffer tmp;
	    struct v4l2_buffer buf;
        CLEAR (buf);

	    buf.type = V4L2_BUF_TYPE_VIDEO_CAPTURE;
        buf.memory = V4L2_MEMORY_MMAP;
        buf.index = i;

	    if (xioctl (fd_, VIDIOC_QUERYBUF, &buf) == -1)
	    {
			AVG_TRACE(Logger::ERROR, "VIDIOC_QUERYBUF index=" << i);
			exit (-1);
	    }

        tmp.length = buf.length;
        tmp.start =
        mmap (NULL /* start anywhere */,
          buf.length,
          PROT_READ | PROT_WRITE /* required */,
          MAP_SHARED /* recommended */,
          fd_, buf.m.offset);

        if (MAP_FAILED == tmp.start)
	    {
			AVG_TRACE(Logger::ERROR, "mmap() failed on buffer index=" << i);
			exit (-1);
	    }
 	   		
   		buffers_.push_back(tmp);
   		AVG_TRACE(Logger::APP, "mmap() buffer index=" << i);
    }
}

void V4LCamera::init_userp(unsigned int buffer_size)
{
	struct v4l2_requestbuffers req;
	CLEAR (req);

    req.count = 4;
    req.type = V4L2_BUF_TYPE_VIDEO_CAPTURE;
    req.memory = V4L2_MEMORY_USERPTR;

    if (xioctl(fd_, VIDIOC_REQBUFS, &req) == -1) {
    	if (EINVAL == errno) {
        	fprintf (stderr, "%s does not support "
                                 "user pointer i/o\n", m_sDevice.c_str());
            //exit (EXIT_FAILURE);
        } else {
            //errno_exit ("VIDIOC_REQBUFS");
        }
    }

    buffers_.clear();

    for (int i=0; i<4; ++i) {
    	Buffer tmp;
        tmp.length = buffer_size;
        tmp.start = malloc (buffer_size);

        if (!tmp.start) {
			fprintf (stderr, "Out of memory\n");
    		//exit (EXIT_FAILURE);
		}
		
		buffers_.push_back(tmp);
    }
}
