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

#include "GLColorShader.h"
#include "ShaderRegistry.h"

#include "../base/Logger.h"
#include "../base/Exception.h"

#include <iostream>

using namespace std;

#define COLORSPACE_SHADER "color"

namespace avg {

GLColorShader::GLColorShader()
{
    m_pShader = getShader(COLORSPACE_SHADER);
    m_pColorModelParam = m_pShader->getParam<int>("colorModel");

    m_pColorCoeff0Param = m_pShader->getParam<glm::vec4>("colorCoeff0");
    m_pColorCoeff1Param = m_pShader->getParam<glm::vec4>("colorCoeff1");
    m_pColorCoeff2Param = m_pShader->getParam<glm::vec4>("colorCoeff2");
    m_pColorCoeff3Param = m_pShader->getParam<glm::vec4>("colorCoeff3");
    m_pGammaParam = m_pShader->getParam<glm::vec4>("gamma");
 
    m_pUseColorCoeffParam = m_pShader->getParam<int>("bUseColorCoeff");
    m_pPremultipliedAlphaParam = m_pShader->getParam<int>("bPremultipliedAlpha");
    m_pUseMaskParam = m_pShader->getParam<int>("bUseMask");
    m_pMaskPosParam = m_pShader->getParam<glm::vec2>("maskPos");
    m_pMaskSizeParam = m_pShader->getParam<glm::vec2>("maskSize");

    m_pShader->activate();
    m_pShader->getParam<int>("texture")->set(0);
    m_pShader->getParam<int>("cbTexture")->set(1);
    m_pShader->getParam<int>("crTexture")->set(2);
    m_pShader->getParam<int>("aTexture")->set(3);
    m_pShader->getParam<int>("maskTexture")->set(4);
}

GLColorShader::~GLColorShader()
{
}

void GLColorShader::activate()
{
    m_pShader->activate();
}

void GLColorShader::setColorModel(int model)
{
    m_pColorModelParam->set(model);
}

void GLColorShader::setColorspaceMatrix(const glm::mat4& mat)
{
    m_pColorCoeff0Param->set(glm::vec4(mat[0][0], mat[0][1], mat[0][2], 0));
    m_pColorCoeff1Param->set(glm::vec4(mat[1][0], mat[1][1], mat[1][2], 0));
    m_pColorCoeff2Param->set(glm::vec4(mat[2][0], mat[2][1], mat[2][2], 0));
    m_pColorCoeff3Param->set(glm::vec4(mat[3][0], mat[3][1], mat[3][2], 1));
    m_pUseColorCoeffParam->set(true);
}

void GLColorShader::disableColorspaceMatrix()
{
    m_pUseColorCoeffParam->set(false);
}

void GLColorShader::setGamma(const glm::vec4& gamma)
{
    m_pGammaParam->set(gamma);
}

void GLColorShader::setPremultipliedAlpha(bool bPremultipliedAlpha)
{
    m_pPremultipliedAlphaParam->set(bPremultipliedAlpha);
}

void GLColorShader::setMask(bool bUseMask, const glm::vec2& maskPos,
        const glm::vec2& maskSize)
{
    m_pUseMaskParam->set(bUseMask);
    if (bUseMask) {
        m_pMaskPosParam->set(maskPos);
        m_pMaskSizeParam->set(maskSize);
    }
}

void GLColorShader::createShader()
{
    avg::createShader(COLORSPACE_SHADER);
}

}
