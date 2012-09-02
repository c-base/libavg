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

#ifndef _OGLHelper_H_
#define _OGLHelper_H_

#include "../api.h"
#ifdef _WIN32
#include <windows.h>
#undef ERROR
#undef WARNING
#include <GL/gl.h>
#include <GL/glu.h>
#include "GL/glext.h"
#else
#include "GL/gl.h"
#include "GL/glu.h"
#endif
#ifdef linux
#define GLX_GLXEXT_PROTOTYPES
#include <GL/glx.h>
#endif
#ifdef _WIN32
//TODO: Does anyone know where these are declared?
typedef void (APIENTRY *PFNWGLEXTSWAPCONTROLPROC) (int);
typedef int (*PFNWGLEXTGETSWAPINTERVALPROC) (void);
#endif

#ifdef linux
// XXX: Hack for compatibility with Ogre3D. Ogre's current gl headers don't define
// this function.
#ifndef PFNGLXSWAPINTERVALEXTPROC
typedef int ( * PFNGLXSWAPINTERVALEXTPROC) (Display *dpy, GLXDrawable drawable, int interval);
#endif
#endif

// For NVX_gpu_memory_info
#ifndef GPU_MEMORY_INFO_DEDICATED_VIDMEM_NVX
    #define GPU_MEMORY_INFO_DEDICATED_VIDMEM_NVX          0x9047
    #define GPU_MEMORY_INFO_TOTAL_AVAILABLE_MEMORY_NVX    0x9048
    #define GPU_MEMORY_INFO_CURRENT_AVAILABLE_VIDMEM_NVX  0x9049
    #define GPU_MEMORY_INFO_EVICTION_COUNT_NVX            0x904A
    #define GPU_MEMORY_INFO_EVICTED_MEMORY_NVX            0x904B
#endif

#include <string>

namespace avg {

#ifdef _WIN32
void AVG_API winOGLErrorCheck(BOOL bOK, const std::string& sWhere);
#endif
bool AVG_API queryOGLExtension(const char* extName);
bool AVG_API queryGLXExtension(const char* extName);
void AVG_API getGLVersion(int& major, int& minor);
void AVG_API getGLShadingLanguageVersion(int& major, int& minor);
std::string AVG_API oglModeToString(int mode);

enum OGLMemoryMode { 
    MM_OGL,  // Standard OpenGL
    MM_PBO   // pixel buffer objects
};

std::string oglMemoryMode2String(OGLMemoryMode mode);

void AVG_API clearGLBuffers(GLbitfield mask);

typedef void (*GLfunction)();
GLfunction AVG_API getFuzzyProcAddress(const char * psz);
#ifdef linux
GLfunction getglXProcAddress(const char * psz);
#endif

namespace glproc {
    extern AVG_API PFNGLGENBUFFERSPROC GenBuffers;
    extern AVG_API PFNGLBUFFERDATAPROC BufferData;
    extern AVG_API PFNGLBUFFERSUBDATAPROC BufferSubData;
    extern AVG_API PFNGLDELETEBUFFERSPROC DeleteBuffers;
    extern AVG_API PFNGLBINDBUFFERPROC BindBuffer;
    extern AVG_API PFNGLMAPBUFFERPROC MapBuffer;
    extern AVG_API PFNGLUNMAPBUFFERPROC UnmapBuffer;
    extern AVG_API PFNGLGETBUFFERSUBDATAPROC GetBufferSubData;

    extern AVG_API PFNGLCREATESHADEROBJECTARBPROC CreateShaderObject;
    extern AVG_API PFNGLSHADERSOURCEARBPROC ShaderSource;
    extern AVG_API PFNGLCOMPILESHADERARBPROC CompileShader;
    extern AVG_API PFNGLCREATEPROGRAMOBJECTARBPROC CreateProgramObject;
    extern AVG_API PFNGLATTACHOBJECTARBPROC AttachObject;
    extern AVG_API PFNGLLINKPROGRAMARBPROC LinkProgram;
    extern AVG_API PFNGLGETOBJECTPARAMETERIVARBPROC GetObjectParameteriv;
    extern AVG_API PFNGLGETINFOLOGARBPROC GetInfoLog;
    extern AVG_API PFNGLUSEPROGRAMOBJECTARBPROC UseProgramObject;
    extern AVG_API PFNGLGETUNIFORMLOCATIONARBPROC GetUniformLocation;
    extern AVG_API PFNGLUNIFORM1IARBPROC Uniform1i;
    extern AVG_API PFNGLUNIFORM1FARBPROC Uniform1f;
    extern AVG_API PFNGLUNIFORM2FARBPROC Uniform2f;
    extern AVG_API PFNGLUNIFORM3FARBPROC Uniform3f;
    extern AVG_API PFNGLUNIFORM4FARBPROC Uniform4f;
    extern AVG_API PFNGLUNIFORM1FVARBPROC Uniform1fv;
    extern AVG_API PFNGLUNIFORMMATRIX4FVARBPROC UniformMatrix4fv;

    extern AVG_API PFNGLBLENDFUNCSEPARATEPROC BlendFuncSeparate;
    extern AVG_API PFNGLBLENDEQUATIONPROC BlendEquation;
    extern AVG_API PFNGLBLENDCOLORPROC BlendColor;
    extern AVG_API PFNGLACTIVETEXTUREPROC ActiveTexture;
    extern AVG_API PFNGLGENERATEMIPMAPEXTPROC GenerateMipmap;

    extern AVG_API PFNGLCHECKFRAMEBUFFERSTATUSEXTPROC CheckFramebufferStatus;
    extern AVG_API PFNGLGENFRAMEBUFFERSEXTPROC GenFramebuffers;
    extern AVG_API PFNGLBINDFRAMEBUFFEREXTPROC BindFramebuffer;
    extern AVG_API PFNGLFRAMEBUFFERTEXTURE2DEXTPROC FramebufferTexture2D;
    extern AVG_API PFNGLDELETEFRAMEBUFFERSEXTPROC DeleteFramebuffers;
    extern AVG_API PFNGLGENRENDERBUFFERSEXTPROC GenRenderbuffers;
    extern AVG_API PFNGLBINDRENDERBUFFEREXTPROC BindRenderbuffer;
    extern AVG_API PFNGLRENDERBUFFERSTORAGEEXTPROC RenderbufferStorage;
    extern AVG_API PFNGLRENDERBUFFERSTORAGEMULTISAMPLEEXTPROC
            RenderbufferStorageMultisample;
    extern AVG_API PFNGLFRAMEBUFFERRENDERBUFFEREXTPROC FramebufferRenderbuffer;
    extern AVG_API PFNGLBLITFRAMEBUFFEREXTPROC BlitFramebuffer;
    extern AVG_API PFNGLDELETERENDERBUFFERSEXTPROC DeleteRenderbuffers;
    extern AVG_API PFNGLDRAWBUFFERSPROC DrawBuffers;
    extern AVG_API PFNGLDRAWRANGEELEMENTSPROC DrawRangeElements;
    
#ifdef linux
    extern PFNGLXSWAPINTERVALEXTPROC SwapIntervalEXT;
#endif
#ifdef _WIN32
    extern AVG_API PFNWGLEXTSWAPCONTROLPROC SwapIntervalEXT;
#endif
    void init();

    extern void * s_hGLLib;
}


}

#endif
 
