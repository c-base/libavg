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
#include "avgconfig.h"

#ifdef _WIN32
    #include <windows.h>
    #undef ERROR
    #undef WARNING
    #include <GL/gl.h>
    #include <GL/glu.h>
    #include "GL/glext.h"
#else
    #ifdef AVG_ENABLE_EGL
        #include <EGL/egl.h>
        #include <GLES2/gl2.h>
        #include <GLES2/gl2ext.h>
    #else
        #include "GL/gl.h"
        #include "GL/glu.h"
        #include "GL/glext.h"
    #endif
#endif
#if defined(linux) && !defined(AVG_ENABLE_EGL)
        #define GLX_GLXEXT_PROTOTYPES
        #include "GL/glx.h"
#endif

#ifdef _WIN32
    //TODO: Does anyone know where these are declared?
    typedef void (APIENTRY *PFNWGLEXTSWAPCONTROLPROC) (int);
    typedef int (*PFNWGLEXTGETSWAPINTERVALPROC) (void);
#endif

#ifdef linux
    #ifndef GLX_CONTEXT_ES2_PROFILE_BIT_EXT
        #define GLX_CONTEXT_ES2_PROFILE_BIT_EXT 0x00000004
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

bool AVG_API queryOGLExtension(const char* extName);
bool AVG_API queryGLXExtension(const char* extName);
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

#ifdef AVG_ENABLE_EGL
        typedef void (GL_APIENTRYP PFNGLGENBUFFERSPROC) (GLsizei n, GLuint *buffers);
        typedef void (GL_APIENTRYP PFNGLBUFFERDATAPROC) (GLenum target, GLsizeiptr size, const GLvoid* data, GLenum usage);
        /*
        typedef void (GL_APIENTRYP PFNGLBUFFERSUBDATAPROC) (GLenum target, GLintptr offset, GLsizeiptr size, const GLvoid* data);
        */
        typedef void (GL_APIENTRYP PFNGLDELETEBUFFERSPROC) (GLsizei n, const GLuint* buffers);
        typedef void (GL_APIENTRYP PFNGLBINDBUFFERPROC) (GLenum target, GLuint buffer);
        typedef void* (GL_APIENTRYP PFNGLMAPBUFFERPROC) (GLenum target, GLenum access);
        typedef GLboolean (GL_APIENTRYP PFNGLUNMAPBUFFERPROC) (GLenum target);
        typedef GLuint (GL_APIENTRYP PFNGLCREATESHADERPROC) (GLenum type);
        typedef void (GL_APIENTRYP PFNGLSHADERSOURCEPROC) (GLuint shader, GLsizei count, const GLchar** string, const GLint* length);
        typedef void (GL_APIENTRYP PFNGLCOMPILESHADERPROC) (GLuint shader);
        typedef GLuint (GL_APIENTRYP PFNGLCREATEPROGRAMPROC) (void);
        typedef void (GL_APIENTRYP PFNGLATTACHSHADERPROC) (GLuint program, GLuint shader);
        typedef void (GL_APIENTRYP PFNGLLINKPROGRAMPROC) (GLuint program);
        typedef void (GL_APIENTRYP PFNGLGETSHADERIVPROC) (GLuint shader, GLenum pname, GLint* params);
        typedef void (GL_APIENTRYP PFNGLGETSHADERINFOLOGPROC) (GLuint shader, GLsizei bufsize, GLsizei* length, GLchar* infolog);
        typedef void (GL_APIENTRYP PFNGLUSEPROGRAMPROC) (GLuint program);
        typedef int (GL_APIENTRYP PFNGLGETUNIFORMLOCATIONPROC) (GLuint program, const GLchar* name);
        typedef void (GL_APIENTRYP PFNGLUNIFORM1IPROC) (GLint location, GLint x);
        typedef void (GL_APIENTRYP PFNGLUNIFORM1FPROC) (GLint location, GLfloat x);
        typedef void (GL_APIENTRYP PFNGLUNIFORM2FPROC) (GLint location, GLfloat x, GLfloat y);
        typedef void (GL_APIENTRYP PFNGLUNIFORM3FPROC) (GLint location, GLfloat x, GLfloat y, GLfloat z);
        typedef void (GL_APIENTRYP PFNGLUNIFORM4FPROC) (GLint location, GLfloat x, GLfloat y, GLfloat z, GLfloat w);
        typedef void (GL_APIENTRYP PFNGLUNIFORM1FVPROC) (GLint location, GLsizei count, const GLfloat* v);
        typedef void (GL_APIENTRYP PFNGLUNIFORMMATRIX4FVPROC) (GLint location, GLsizei count, GLboolean transpose, const GLfloat* value);
        typedef void (GL_APIENTRYP PFNGLBLENDFUNCSEPARATEPROC) (GLenum srcRGB, GLenum dstRGB, GLenum srcAlpha, GLenum dstAlpha);
        typedef void (GL_APIENTRYP PFNGLBLENDEQUATIONPROC) ( GLenum mode );
        typedef void (GL_APIENTRYP PFNGLBLENDCOLORPROC) (GLclampf red, GLclampf green, GLclampf blue, GLclampf alpha);
        typedef void (GL_APIENTRYP PFNGLACTIVETEXTUREPROC) (GLenum texture);
        typedef void (GL_APIENTRYP PFNGLGENERATEMIPMAPPROC) (GLenum target);
        typedef GLenum (GL_APIENTRYP PFNGLCHECKFRAMEBUFFERSTATUSPROC) (GLenum target);
        typedef void (GL_APIENTRYP PFNGLGENFRAMEBUFFERSPROC) (GLsizei n, GLuint* framebuffers);
        typedef void (GL_APIENTRYP PFNGLBINDFRAMEBUFFERPROC) (GLenum target, GLuint framebuffer);
        typedef void (GL_APIENTRYP PFNGLFRAMEBUFFERTEXTURE2DPROC) (GLenum target, GLenum attachment, GLenum textarget, GLuint texture, GLint level);
        typedef void (GL_APIENTRYP PFNGLDELETEFRAMEBUFFERSPROC) (GLsizei n, const GLuint* framebuffers);
        typedef void (GL_APIENTRYP PFNGLGENRENDERBUFFERSPROC) (GLsizei n, GLuint* renderbuffers);
        typedef void (GL_APIENTRYP PFNGLBINDRENDERBUFFERPROC) (GLenum target, GLuint renderbuffer);
        typedef void (GL_APIENTRYP PFNGLRENDERBUFFERSTORAGEPROC) (GLenum target, GLenum internalformat, GLsizei width, GLsizei height);
        typedef void (GL_APIENTRYP PFNGLRENDERBUFFERSTORAGEMULTISAMPLEPROC) (GLenum target, GLsizei samples,  GLenum internalformat, GLsizei width, GLsizei height);
        typedef void (GL_APIENTRYP PFNGLFRAMEBUFFERRENDERBUFFERPROC) (GLenum target, GLenum attachment, GLenum renderbuffertarget, GLuint renderbuffer);
        //TODO: BLITFRAMEBUFFER
        typedef void (GL_APIENTRYP PFNGLDELETERENDERBUFFERSPROC) (GLsizei n, const GLuint* renderbuffers);
        typedef void (GL_APIENTRYP PFNGLVERTEXATTRIBPOINTERPROC) (GLuint indx, GLint size, GLenum type, GLboolean normalized, GLsizei stride, const GLvoid* ptr);
        typedef void (GL_APIENTRYP PFNGLENABLEVERTEXATTRIBARRAYPROC) (GLuint index);
        typedef void (GL_APIENTRYP PFNGLBINDATTRIBLOCATIONPROC) (GLuint program, GLuint index, const GLchar* name);


#endif

namespace glproc {
        extern AVG_API PFNGLGENBUFFERSPROC GenBuffers;
        extern AVG_API PFNGLBUFFERDATAPROC BufferData;
        #ifndef AVG_ENABLE_EGL
        extern AVG_API PFNGLBUFFERSUBDATAPROC BufferSubData;
        extern AVG_API PFNGLGETBUFFERSUBDATAPROC GetBufferSubData;
        extern AVG_API PFNGLDRAWBUFFERSPROC DrawBuffers;
        extern AVG_API PFNGLDRAWRANGEELEMENTSPROC DrawRangeElements;
        extern AVG_API PFNGLDEBUGMESSAGECALLBACKARBPROC DebugMessageCallback;
        extern AVG_API PFNGLBLITFRAMEBUFFERPROC BlitFramebuffer;
        #endif
        extern AVG_API PFNGLDELETEBUFFERSPROC DeleteBuffers;
        extern AVG_API PFNGLBINDBUFFERPROC BindBuffer;
        extern AVG_API PFNGLMAPBUFFERPROC MapBuffer;
        extern AVG_API PFNGLUNMAPBUFFERPROC UnmapBuffer;

        extern AVG_API PFNGLCREATESHADERPROC CreateShader;
        extern AVG_API PFNGLSHADERSOURCEPROC ShaderSource;
        extern AVG_API PFNGLCOMPILESHADERPROC CompileShader;
        extern AVG_API PFNGLCREATEPROGRAMPROC CreateProgram;
        extern AVG_API PFNGLATTACHSHADERPROC AttachShader;
        extern AVG_API PFNGLLINKPROGRAMPROC LinkProgram;
        extern AVG_API PFNGLGETSHADERIVPROC GetShaderiv;
        extern AVG_API PFNGLGETSHADERINFOLOGPROC GetShaderInfoLog;
        extern AVG_API PFNGLUSEPROGRAMPROC UseProgram;
        extern AVG_API PFNGLGETUNIFORMLOCATIONPROC GetUniformLocation;
        extern AVG_API PFNGLUNIFORM1IPROC Uniform1i;
        extern AVG_API PFNGLUNIFORM1FPROC Uniform1f;
        extern AVG_API PFNGLUNIFORM2FPROC Uniform2f;
        extern AVG_API PFNGLUNIFORM3FPROC Uniform3f;
        extern AVG_API PFNGLUNIFORM4FPROC Uniform4f;
        extern AVG_API PFNGLUNIFORM1FVPROC Uniform1fv;
        extern AVG_API PFNGLUNIFORMMATRIX4FVPROC UniformMatrix4fv;

        extern AVG_API PFNGLBLENDFUNCSEPARATEPROC BlendFuncSeparate;
        extern AVG_API PFNGLBLENDEQUATIONPROC BlendEquation;
        extern AVG_API PFNGLBLENDCOLORPROC BlendColor;
        extern AVG_API PFNGLACTIVETEXTUREPROC ActiveTexture;
        extern AVG_API PFNGLGENERATEMIPMAPPROC GenerateMipmap;

        extern AVG_API PFNGLCHECKFRAMEBUFFERSTATUSPROC CheckFramebufferStatus;
        extern AVG_API PFNGLGENFRAMEBUFFERSPROC GenFramebuffers;
        extern AVG_API PFNGLBINDFRAMEBUFFERPROC BindFramebuffer;
        extern AVG_API PFNGLFRAMEBUFFERTEXTURE2DPROC FramebufferTexture2D;
        extern AVG_API PFNGLDELETEFRAMEBUFFERSPROC DeleteFramebuffers;
        extern AVG_API PFNGLGENRENDERBUFFERSPROC GenRenderbuffers;
        extern AVG_API PFNGLBINDRENDERBUFFERPROC BindRenderbuffer;
        extern AVG_API PFNGLRENDERBUFFERSTORAGEPROC RenderbufferStorage;
        extern AVG_API PFNGLRENDERBUFFERSTORAGEMULTISAMPLEPROC
                RenderbufferStorageMultisample;
        extern AVG_API PFNGLFRAMEBUFFERRENDERBUFFERPROC FramebufferRenderbuffer;
        extern AVG_API PFNGLDELETERENDERBUFFERSPROC DeleteRenderbuffers;

        extern AVG_API PFNGLVERTEXATTRIBPOINTERPROC VertexAttribPointer;
        extern AVG_API PFNGLENABLEVERTEXATTRIBARRAYPROC EnableVertexAttribArray;
        extern AVG_API PFNGLBINDATTRIBLOCATIONPROC BindAttribLocation;
        #ifdef linux
            #ifndef AVG_ENABLE_EGL
            extern PFNGLXSWAPINTERVALEXTPROC SwapIntervalEXT;
            #endif
        #endif
#ifdef _WIN32
    extern AVG_API PFNWGLEXTSWAPCONTROLPROC SwapIntervalEXT;
#endif
    void init();

    extern void * s_hGLLib;
}


}

#endif
 
