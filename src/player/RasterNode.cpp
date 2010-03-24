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

#include "RasterNode.h"

#include "NodeDefinition.h"
#include "SDLDisplayEngine.h"
#include "OGLSurface.h"

#include "../base/MathHelper.h"
#include "../base/Logger.h"
#include "../base/XMLHelper.h"
#include "../base/Exception.h"

#include <Magick++.h>

using namespace std;

namespace avg {

NodeDefinition RasterNode::createDefinition()
{
    return NodeDefinition("rasternode")
        .extendDefinition(AreaNode::createDefinition())
        .addArg(Arg<int>("maxtilewidth", -1, false, 
                offsetof(RasterNode, m_MaxTileSize.x)))
        .addArg(Arg<int>("maxtileheight", -1, false, 
                offsetof(RasterNode, m_MaxTileSize.y)))
        .addArg(Arg<string>("blendmode", "blend", false, 
                offsetof(RasterNode, m_sBlendMode)))
        .addArg(Arg<bool>("mipmap", false))
        .addArg(Arg<UTF8String>("maskhref", "", false, offsetof(RasterNode, m_sMaskHref)))
        .addArg(Arg<DPoint>("maskpos", DPoint(0,0), false,
                offsetof(RasterNode, m_MaskPos)))
        .addArg(Arg<DPoint>("masksize", DPoint(0,0), false,
                offsetof(RasterNode, m_MaskSize)));
}

RasterNode::RasterNode()
    : m_pSurface(0),
      m_Material(GL_CLAMP_TO_EDGE, GL_CLAMP_TO_EDGE, false),
      m_bBound(false),
      m_TileSize(-1,-1),
      m_pVertexes(0)
{
}

RasterNode::~RasterNode()
{
    if (m_pSurface) {
        delete m_pSurface;
        m_pSurface = 0;
    }
}

void RasterNode::setArgs(const ArgList& Args)
{
    AreaNode::setArgs(Args);
    if ((!ispow2(m_MaxTileSize.x) && m_MaxTileSize.x != -1)
            || (!ispow2(m_MaxTileSize.y) && m_MaxTileSize.y != -1)) 
    {
        throw Exception(AVG_ERR_OUT_OF_RANGE, 
                "maxtilewidth and maxtileheight must be powers of two.");
    }
    m_Material.setUseMipmaps(Args.getArgVal<bool>("mipmap"));
}

void RasterNode::setRenderingEngines(DisplayEngine * pDisplayEngine, 
        AudioEngine * pAudioEngine)
{
    AreaNode::setRenderingEngines(pDisplayEngine, pAudioEngine);

    getSurface();
    m_pSurface->attach(dynamic_cast<SDLDisplayEngine*>(pDisplayEngine));
    m_bBound = false;
    if (m_MaxTileSize != IntPoint(-1, -1)) {
        m_TileSize = m_MaxTileSize;
    }
    calcVertexGrid(m_TileVertices);
    m_pSurface->setMaterial(m_Material);
    setBlendModeStr(m_sBlendMode);
    if (m_Material.getHasMask()) {
        m_pSurface->createMask(m_pMaskBmp->getSize());
        downloadMask();
    }
    
}

void RasterNode::disconnect(bool bKill)
{
    if (m_pVertexes) {
        delete m_pVertexes;
        m_pVertexes = 0;
    }
    if (m_pSurface) {
        m_pSurface->destroy();
    }
    AreaNode::disconnect(bKill);
}

void RasterNode::checkReload()
{
    m_bBound = false;
    string sLastMaskFilename = m_sMaskFilename;
    string sMaskFilename = m_sMaskHref;
    initFilename(sMaskFilename);
    if (sLastMaskFilename != sMaskFilename) {
        m_sMaskFilename = sMaskFilename;
        try {
            if (m_sMaskFilename != "") {
                AVG_TRACE(Logger::MEMORY, "Loading " << m_sMaskFilename);
                m_pMaskBmp = BitmapPtr(new Bitmap(m_sMaskFilename));
                calcMaskPos();
            }
        } catch (Magick::Exception & ex) {
            m_sMaskFilename = "";
            if (getState() != Node::NS_UNCONNECTED) {
                AVG_TRACE(Logger::ERROR, ex.what());
            } else {
                AVG_TRACE(Logger::MEMORY, ex.what());
            }
        }
        if (m_sMaskFilename == "") {
            m_pMaskBmp = BitmapPtr();
            m_Material.setMask(false);
            setMaterial(m_Material);
        }
        if (getState() == Node::NS_CANRENDER && m_Material.getHasMask()) {
            m_pSurface->createMask(m_pMaskBmp->getSize());
            downloadMask();
        }
    } else {
        calcMaskPos();
    }
}

VertexGrid RasterNode::getOrigVertexCoords()
{
    checkDisplayAvailable("getOrigVertexCoords");
    if (!m_bBound) {
        bind();
    }
    VertexGrid grid;
    calcVertexGrid(grid);
    return grid;
}

VertexGrid RasterNode::getWarpedVertexCoords() 
{
    checkDisplayAvailable("getWarpedVertexCoords");
    if (!m_bBound) {
        bind();
    }
    return m_TileVertices;
}

void RasterNode::setWarpedVertexCoords(const VertexGrid& grid)
{
    checkDisplayAvailable("setWarpedVertexCoords");
    if (!m_bBound) {
        bind();
    }
    bool bGridOK = true;
    IntPoint numTiles = getNumTiles();
    if (grid.size() != (unsigned)(numTiles.y+1)) {
        bGridOK = false;
    }
    for (unsigned i = 0; i<grid.size(); ++i) {
        if (grid[i].size() != (unsigned)(numTiles.x+1)) {
            bGridOK = false;
        }
    }
    if (!bGridOK) {
        throw Exception(AVG_ERR_OUT_OF_RANGE, 
                "setWarpedVertexCoords() called with incorrect grid size.");
    }
    m_TileVertices = grid;
}

int RasterNode::getMaxTileWidth() const
{
    return m_MaxTileSize.x;
}

int RasterNode::getMaxTileHeight() const
{
    return m_MaxTileSize.y;
}

bool RasterNode::getMipmap() const
{
   return m_Material.getUseMipmaps();
}

const std::string& RasterNode::getBlendModeStr() const
{
    return m_sBlendMode;
}

void RasterNode::setBlendModeStr(const std::string& sBlendMode)
{
    m_sBlendMode = sBlendMode;
    m_BlendMode = DisplayEngine::stringToBlendMode(sBlendMode);
}

const UTF8String& RasterNode::getMaskHRef() const
{
    return m_sMaskHref;
}

void RasterNode::setMaskHRef(const UTF8String& href)
{
    m_sMaskHref = href;
    checkReload();
}

const DPoint& RasterNode::getMaskPos() const
{
    return m_MaskPos;
}

void RasterNode::setMaskPos(const DPoint& pos)
{
    m_MaskPos = pos;
    calcMaskPos();
}

const DPoint& RasterNode::getMaskSize() const
{
    return m_MaskSize;
}

void RasterNode::setMaskSize(const DPoint& size)
{
    m_MaskSize = size;
    calcMaskPos();
}

NodePtr RasterNode::getElementByPos(const DPoint & pos)
{
    // Node isn't pickable if it's warped.
    if (m_MaxTileSize == IntPoint(-1, -1)) {
        return AreaNode::getElementByPos(pos);
    } else {
        return NodePtr();
    }
}

BitmapPtr RasterNode::getBitmap()
{
    if (m_pSurface) {
        return m_pSurface->readbackBmp();
    } else {
        return BitmapPtr(); 
    }
}

void RasterNode::blt32(const DPoint& DestSize, double opacity, 
        DisplayEngine::BlendMode Mode)
{
    glColor4d(1.0, 1.0, 1.0, opacity);
    blt(DestSize, Mode);
}

void RasterNode::blta8(const DPoint& DestSize, double opacity, 
        const Pixel32& color, DisplayEngine::BlendMode Mode)
{
    glColor4d(double(color.getR())/256, double(color.getG())/256, 
            double(color.getB())/256, opacity);
    blt(DestSize, Mode);
}

DisplayEngine::BlendMode RasterNode::getBlendMode() const
{
    return m_BlendMode;
}

OGLSurface * RasterNode::getSurface()
{
    if (!m_pSurface) {
        m_pSurface = new OGLSurface(m_Material);
    }
    return m_pSurface;
}

const MaterialInfo& RasterNode::getMaterial() const
{
    return m_Material;
}

void RasterNode::calcMaskPos()
{
    if (m_sMaskFilename != "") {
        setMaterialMask(m_Material, m_MaskPos, m_MaskSize, DPoint(getMediaSize()));
        setMaterial(m_Material);
    }
}

void RasterNode::bind() 
{
    if (!m_bBound) {
        calcTexCoords();
    }
    m_pSurface->downloadTexture();
    m_bBound = true;
}

void RasterNode::setMaterialMask(MaterialInfo& material, const DPoint& pos, 
        const DPoint& size, const DPoint& mediaSize)
{
    DPoint maskSize;
    DPoint maskPos;
    if (size == DPoint(0,0)) {
        maskSize = DPoint(1,1);
    } else {
        maskSize = DPoint(size.x/mediaSize.x, size.y/mediaSize.y);
    }
    maskPos = DPoint(pos.x/mediaSize.x, pos.y/mediaSize.y);
    material.setMask(true);
    material.setMaskCoords(maskPos, maskSize);
}

void RasterNode::setMaterial(const MaterialInfo& material)
{
    m_Material = material;
    getSurface()->setMaterial(m_Material);
}

void RasterNode::downloadMask()
{
    BitmapPtr pBmp = m_pSurface->lockMaskBmp();
    pBmp->copyPixels(*m_pMaskBmp);
    m_pSurface->unlockMaskBmp();
    m_pSurface->downloadTexture();
}

void RasterNode::checkDisplayAvailable(std::string sMsg)
{
    if (!(getState() == Node::NS_CANRENDER)) {
        throw Exception(AVG_ERR_UNSUPPORTED,
            string(sMsg) + ": cannot access vertex coordinates before Player.play().");
    }
}

void RasterNode::blt(const DPoint& destSize, DisplayEngine::BlendMode mode)
{
    if (!m_bBound) {
        bind();
    }
    getDisplayEngine()->enableGLColorArray(false);
    getDisplayEngine()->enableTexture(true);
    getDisplayEngine()->setBlendMode(mode);
    glPushMatrix();
    glScaled(destSize.x, destSize.y, 1);

    m_pVertexes->reset();
    for (unsigned y=0; y<m_TileVertices.size()-1; y++) {
        for (unsigned x=0; x<m_TileVertices[0].size()-1; x++) {
            int curVertex=m_pVertexes->getCurVert();
            m_pVertexes->appendPos(m_TileVertices[y][x], m_TexCoords[y][x]); 
            m_pVertexes->appendPos(m_TileVertices[y][x+1], m_TexCoords[y][x+1]); 
            m_pVertexes->appendPos(m_TileVertices[y+1][x+1], m_TexCoords[y+1][x+1]); 
            m_pVertexes->appendPos(m_TileVertices[y+1][x], m_TexCoords[y+1][x]); 
            m_pVertexes->appendQuadIndexes(
                    curVertex+1, curVertex, curVertex+2, curVertex+3);
        }
    }

    m_pSurface->activate(getMediaSize());
    m_pVertexes->draw();
    m_pSurface->deactivate();

    glPopMatrix();

    PixelFormat pf = m_pSurface->getPixelFormat();
    AVG_TRACE(Logger::BLTS, "(" << destSize.x << ", " 
            << destSize.y << ")" << ", m_pf: " 
            << Bitmap::getPixelFormatString(pf) << ", " 
            << oglModeToString(getDisplayEngine()->getOGLSrcMode(pf)) << "-->" 
            << oglModeToString(getDisplayEngine()->getOGLDestMode(pf)));
}

IntPoint RasterNode::getNumTiles()
{
    IntPoint size = m_pSurface->getSize();
    if (m_TileSize.x == -1) {
        return IntPoint(1,1);
    } else {
        return IntPoint(safeCeil(double(size.x)/m_TileSize.x),
                safeCeil(double(size.y)/m_TileSize.y));
    }
}

void RasterNode::calcVertexGrid(VertexGrid& grid)
{
    IntPoint numTiles = getNumTiles();
    std::vector<DPoint> TileVerticesLine(numTiles.x+1);
    grid = std::vector<std::vector<DPoint> > (numTiles.y+1, TileVerticesLine);
    for (unsigned y=0; y<grid.size(); y++) {
        for (unsigned x=0; x<grid[y].size(); x++) {
            calcTileVertex(x, y, grid[y][x]);
        }
    }
    if (m_pVertexes) {
        delete m_pVertexes;
    }
    m_pVertexes = new VertexArray(numTiles.x*numTiles.y*4, numTiles.x*numTiles.y*6);
}

void RasterNode::calcTileVertex(int x, int y, DPoint& Vertex) 
{
    IntPoint numTiles = getNumTiles();
    if (x < numTiles.x) {
        Vertex.x = double(m_TileSize.x*x) / m_pSurface->getSize().x;
    } else {
        Vertex.x = 1;
    }
    if (y < numTiles.y) {
        Vertex.y = double(m_TileSize.y*y) / m_pSurface->getSize().y;
    } else {
        Vertex.y = 1;
    }
}

void RasterNode::calcTexCoords()
{
    DPoint textureSize = DPoint(m_pSurface->getTextureSize());
    DPoint imageSize = DPoint(m_pSurface->getSize());
    DPoint texCoordExtents = DPoint(imageSize.x/textureSize.x,
            imageSize.y/textureSize.y);

    DPoint texSizePerTile;
    if (m_TileSize.x == -1) {
        texSizePerTile = texCoordExtents;
    } else {
        texSizePerTile = DPoint(double(m_TileSize.x)/imageSize.x*texCoordExtents.x,
                double(m_TileSize.y)/imageSize.y*texCoordExtents.y);
    }

    IntPoint numTiles = getNumTiles();
    vector<DPoint> texCoordLine(numTiles.x+1);
    m_TexCoords = std::vector<std::vector<DPoint> > 
            (numTiles.y+1, texCoordLine);
    for (unsigned y=0; y<m_TexCoords.size(); y++) {
        for (unsigned x=0; x<m_TexCoords[y].size(); x++) {
            if (y == m_TexCoords.size()-1) {
                m_TexCoords[y][x].y = texCoordExtents.y;
            } else {
                m_TexCoords[y][x].y = texSizePerTile.y*y;
            }
            if (x == m_TexCoords[y].size()-1) {
                m_TexCoords[y][x].x = texCoordExtents.x;
            } else {
                m_TexCoords[y][x].x = texSizePerTile.x*x;
            }
        }
    }
}
}
