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

#include "OGLSurface.h"
#include "SDLDisplayEngine.h"

#include "../base/MathHelper.h"
#include "../base/Logger.h"
#include "../base/Exception.h"
#include "../base/ScopeTimer.h"
#include "../base/ObjectCounter.h"

#include "../graphics/ShaderRegistry.h"

#include <iostream>
#include <sstream>

using namespace std;

#define COLORSPACE_SHADER "COLORSPACE"

namespace avg {

OGLSurface::OGLSurface(const MaterialInfo& material)
    : m_Size(-1,-1),
      m_bUseForeignTexture(false),
      m_Material(material),
      m_pEngine(0)
{
    ObjectCounter::get()->incRef(&typeid(*this));
}

OGLSurface::~OGLSurface()
{
    ObjectCounter::get()->decRef(&typeid(*this));
}

void OGLSurface::attach(SDLDisplayEngine * pEngine)
{
    m_pEngine = pEngine;
    m_MemoryMode = m_pEngine->getMemoryModeSupported();
    if (m_Material.getHasMask() && !getEngine()->isUsingShaders()) {
        throw Exception(AVG_ERR_VIDEO_GENERAL,
                "Can't set mask bitmap since shader support is disabled.");
    }
}

void OGLSurface::create(const IntPoint& size, PixelFormat pf)
{
    AVG_ASSERT(m_pEngine);
    if (m_pTextures[0] && m_Size == size && m_pf == pf) {
        // If nothing's changed, we can ignore everything.
        return;
    }
    m_Size = size;
    m_pf = pf;

    if (m_pf == YCbCr420p || m_pf == YCbCrJ420p) {
        m_pTextures[0] = PBOTexturePtr(new PBOTexture(size, I8, m_Material, m_pEngine,
                m_MemoryMode));
        IntPoint halfSize(size.x/2, size.y/2);
        m_pTextures[1] = PBOTexturePtr(new PBOTexture(halfSize, I8, m_Material, m_pEngine,
                m_MemoryMode));
        m_pTextures[2] = PBOTexturePtr(new PBOTexture(halfSize, I8, m_Material, m_pEngine,
                m_MemoryMode));
    } else {
        m_pTextures[0] = PBOTexturePtr(new PBOTexture(size, m_pf, m_Material, m_pEngine,
                m_MemoryMode));
    }
    m_bUseForeignTexture = false;
}

void OGLSurface::createMask(const IntPoint& size)
{
    AVG_ASSERT(m_pEngine);
    AVG_ASSERT(m_Material.getHasMask());
    m_MaskSize = size;
    m_pMaskTexture = PBOTexturePtr(new PBOTexture(size, I8, m_Material, m_pEngine,
            m_MemoryMode));
}

void OGLSurface::destroy()
{
    m_bUseForeignTexture = false;
    m_pTextures[0] = PBOTexturePtr();
    m_pTextures[1] = PBOTexturePtr();
    m_pTextures[2] = PBOTexturePtr();
}

void OGLSurface::activate(const IntPoint& logicalSize) const
{
    if (useShader()) {
        OGLShaderPtr pShader = getShader(COLORSPACE_SHADER);
        pShader->activate();
        OGLErrorCheck(AVG_ERR_VIDEO_GENERAL, "OGLSurface::activate()");
        switch (m_pf) {
            case YCbCr420p:
                pShader->setUniformIntParam("colorModel", 1);
                break;
            case YCbCrJ420p:
                pShader->setUniformIntParam("colorModel", 2);
                break;
            case A8:
                pShader->setUniformIntParam("colorModel", 3);
                break;
            default:
                pShader->setUniformIntParam("colorModel", 0);
        }

        m_pTextures[0]->activate(GL_TEXTURE0);
        pShader->setUniformIntParam("texture", 0);
        
        if (m_pf == YCbCr420p || m_pf == YCbCrJ420p) {
            m_pTextures[1]->activate(GL_TEXTURE1);
            pShader->setUniformIntParam("cbTexture", 1);
            m_pTextures[2]->activate(GL_TEXTURE2);
            pShader->setUniformIntParam("crTexture", 2);
        }
        
        pShader->setUniformIntParam("bUseMask", m_Material.getHasMask());
        if (m_Material.getHasMask()) {
            m_pMaskTexture->activate(GL_TEXTURE3);
            pShader->setUniformIntParam("maskTexture", 3);
            pShader->setUniformDPointParam("maskPos", m_Material.getMaskPos());
            // maskScale is (1,1) for everything excepting words nodes.
            DPoint maskScale(1,1);
            if (logicalSize != IntPoint(0,0)) {
                maskScale = DPoint((double)logicalSize.x/m_Size.x, 
                        (double)logicalSize.y/m_Size.y);
            }
            pShader->setUniformDPointParam("maskSize", m_Material.getMaskSize()*maskScale);
        }

        OGLErrorCheck(AVG_ERR_VIDEO_GENERAL, "OGLSurface::activate: params");
    } else {
        m_pTextures[0]->activate(GL_TEXTURE0);
        if (m_pEngine->isUsingShaders()) {
            glproc::UseProgramObject(0);
        }
    }
}

void OGLSurface::deactivate() const
{
    if (m_pf == YCbCr420p || m_pf == YCbCrJ420p) {
        glproc::ActiveTexture(GL_TEXTURE1);
        glDisable(GL_TEXTURE_2D);
        glproc::ActiveTexture(GL_TEXTURE2);
        glDisable(GL_TEXTURE_2D);
        glproc::ActiveTexture(GL_TEXTURE0);
        glproc::UseProgramObject(0);
        OGLErrorCheck(AVG_ERR_VIDEO_GENERAL, "OGLSurface::deactivate");
    }
    if (m_Material.getHasMask()) {
        glproc::ActiveTexture(GL_TEXTURE3);
        glDisable(GL_TEXTURE_2D);
        glproc::ActiveTexture(GL_TEXTURE0);
        glproc::UseProgramObject(0);
    }
}

BitmapPtr OGLSurface::lockBmp(int i)
{
    return m_pTextures[i]->lockBmp();
}

void OGLSurface::unlockBmps()
{
    m_pTextures[0]->unlockBmp();
    if (m_pf == YCbCr420p || m_pf == YCbCrJ420p) {
        m_pTextures[1]->unlockBmp();
        m_pTextures[2]->unlockBmp();
    }
}

BitmapPtr OGLSurface::readbackBmp()
{
    return m_pTextures[0]->readbackBmp();
}

void OGLSurface::setTex(GLTexturePtr pTex)
{
    m_bUseForeignTexture = true;
    m_pTextures[0]->setTex(pTex);
}

BitmapPtr OGLSurface::lockMaskBmp()
{
    AVG_ASSERT(m_Material.getHasMask());
    return m_pMaskTexture->lockBmp();
}

void OGLSurface::unlockMaskBmp()
{
    m_pMaskTexture->unlockBmp();
}

const MaterialInfo& OGLSurface::getMaterial() const
{
    return m_Material;
}

void OGLSurface::setMaterial(const MaterialInfo& material)
{
    if (getEngine() && (material.getHasMask() && !getEngine()->isUsingShaders())) {
        throw Exception(AVG_ERR_VIDEO_GENERAL,
                "Can't set mask bitmap since shader support is disabled.");
    }
    bool bOldHasMask = m_Material.getHasMask();
    m_Material = material;
    if (m_pTextures[0]) {
        m_pTextures[0]->setMaterial(material);
        if (m_pf == YCbCr420p || m_pf == YCbCrJ420p) {
            m_pTextures[1]->setMaterial(material);
            m_pTextures[2]->setMaterial(material);
        }
    }
    if (bOldHasMask && !m_Material.getHasMask()) {
        m_pMaskTexture = PBOTexturePtr();
    }
    if (!bOldHasMask && m_Material.getHasMask() && m_pMaskTexture) {
        m_pMaskTexture = PBOTexturePtr(new PBOTexture(m_MaskSize, I8, m_Material, 
                m_pEngine, m_MemoryMode));
    }
}

void OGLSurface::downloadTexture()
{
    if (m_pTextures[0] && !m_bUseForeignTexture) {
        m_pTextures[0]->download();
        if (m_pf == YCbCr420p || m_pf == YCbCrJ420p) {
            m_pTextures[1]->download();
            m_pTextures[2]->download();
        }
    }
}

void OGLSurface::downloadMaskTexture()
{
    if (m_Material.getHasMask()) {
        m_pMaskTexture->download();
    }
}

PixelFormat OGLSurface::getPixelFormat()
{
    return m_pf;
}
        
IntPoint OGLSurface::getSize()
{
    return m_Size;
}

IntPoint OGLSurface::getTextureSize()
{
    return m_pTextures[0]->getTextureSize();
}

bool OGLSurface::isCreated() const
{
    return m_pTextures[0];
}

void OGLSurface::createShader()
{
    string sProgram =
        "uniform sampler2D texture;\n"
        "uniform sampler2D yTexture;\n"
        "uniform sampler2D cbTexture;\n"
        "uniform sampler2D crTexture;\n"
        "uniform sampler2D maskTexture;\n"
        "uniform int colorModel;  // 0=rgb, 1=ycbcr, 2=ycbcrj, 3=greyscale\n"
        "uniform bool bUseMask;\n"
        "uniform vec2 maskPos;\n"
        "uniform vec2 maskSize;\n"
        "\n"
        "vec4 convertYCbCr()\n"
        "{\n"
        "    vec3 ycbcr;\n"
        "    ycbcr.r = texture2D(texture, gl_TexCoord[0].st).r-0.0625;\n"
        "    ycbcr.g = texture2D(cbTexture, (gl_TexCoord[0].st)).r-0.5;\n"
        "    ycbcr.b = texture2D(crTexture, (gl_TexCoord[0].st)).r-0.5;\n"
        "    vec3 rgb;\n"
        "    rgb = ycbcr*mat3(1.16,  0.0,   1.60,\n"
        "                     1.16, -0.39, -0.81,\n"
        "                     1.16,  2.01,  0.0 );\n"
        "    return vec4(rgb, gl_Color.a);\n"
        "}\n"
        "\n"
        "vec4 convertYCbCrJ()\n"
        "{\n"
        "    vec3 ycbcr;\n"
        "    ycbcr.r = texture2D(texture, gl_TexCoord[0].st).r;\n"
        "    ycbcr.g = texture2D(cbTexture, (gl_TexCoord[0].st)).r-0.5;\n"
        "    ycbcr.b = texture2D(crTexture, (gl_TexCoord[0].st)).r-0.5;\n"
        "    vec3 rgb;\n"
        "    rgb = ycbcr*mat3(1,  0.0  , 1.40,\n"
        "                     1, -0.34, -0.71,\n"
        "                     1,  1.77,  0.0 );\n"
        "    return vec4(rgb, gl_Color.a);\n"
        "}\n"
        "\n"
        "void main(void)\n"
        "{\n"
        "    vec4 rgba;\n"
        "    if (colorModel == 0) {\n"
        "        rgba = texture2D(texture, gl_TexCoord[0].st);\n"
        "        rgba.a *= gl_Color.a;\n"
        "    } else if (colorModel == 1) {\n"
        "        rgba = convertYCbCr();\n"
        "    } else if (colorModel == 2) {\n"
        "        rgba = convertYCbCrJ();\n"
        "    } else if (colorModel == 3) {\n"
        "        rgba = gl_Color;\n"
        "        rgba.a *= texture2D(texture, gl_TexCoord[0].st).a;\n"
        "    } else {\n"
        "        rgba = vec4(1,1,1,1);\n"
        "    }\n"
        "    if (bUseMask) {\n"
        "        rgba.a *= texture2D(maskTexture,\n"
        "               (gl_TexCoord[0].st/maskSize)-maskPos).r;\n"
        "    }\n"
        "    gl_FragColor = rgba;\n"
        "}\n";
    getOrCreateShader(COLORSPACE_SHADER, sProgram);
}

SDLDisplayEngine * OGLSurface::getEngine() const
{
    return m_pEngine;
}

bool OGLSurface::useShader() const
{
    return getEngine()->isUsingShaders() && 
            (m_Material.getHasMask() || m_pf == YCbCr420p || m_pf == YCbCrJ420p);
}

}
