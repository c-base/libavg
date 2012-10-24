# -*- coding: utf-8 -*-
# libavg - Media Playback Engine.
# Copyright (C) 2003-2011 Ulrich von Zadow
#
# This library is free software; you can redistribute it and/or
# modify it under the terms of the GNU Lesser General Public
# License as published by the Free Software Foundation; either
# version 2 of the License, or (at your option) any later version.
#
# This library is distributed in the hope that it will be useful,
# but WITHOUT ANY WARRANTY; without even the implied warranty of
# MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
# Lesser General Public License for more details.
#
# You should have received a copy of the GNU Lesser General Public
# License along with this library; if not, write to the Free Software
# Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
#
# Current versions can be found at www.libavg.de

from libavg import avg, player

class Orientation():
    VERTICAL = 0
    HORIZONTAL = 1


class StretchNodeBase(avg.DivNode):
    
    def __init__(self, src=None, parent=None, **kwargs):
        super(StretchNodeBase, self).__init__(**kwargs)
        self.registerInstance(self, parent)
        if isinstance(src, avg.Bitmap):
            self._bmp = src
        else:
            self._bmp = self._bmpFromSrc(src)
        
    def getWidth(self):
        return self._baseWidth

    def setWidth(self, width):
        self._positionNodes(avg.Point2D(width, self._baseHeight))

    _baseWidth = avg.DivNode.width
    width = property(getWidth, setWidth)

    def getHeight(self):
        return self._baseHeight

    def setHeight(self, height):
        self._positionNodes(avg.Point2D(self._baseWidth, height))

    _baseHeight = avg.DivNode.height
    height = property(getHeight, setHeight)

    def getSize(self):
        return self._baseSize

    def setSize(self, size):
        self._positionNodes(avg.Point2D(size))

    _baseSize = avg.DivNode.size
    size = property(getSize, setSize)

    def _initNodes(self):    
        self._setSizeFromBmp(self._bmp)
        self._positionNodes(self._baseSize)

        if player.isPlaying():
            self._renderImages()
        else:
            player.subscribe(avg.Player.PLAYBACK_START, self._renderImages)

    def _bmpFromSrc(self, src):
        if isinstance(src, basestring):
            return avg.Bitmap(src)
        elif isinstance(src, avg.Bitmap):
            return src
        else:
            raise RuntimeError("src must be a string or a Bitmap.")
        
    def _setSizeFromBmp(self, bmp):
        size = bmp.getSize()
        if self._baseSize.x == 0:
            self._baseWidth = size.x
        if self._baseSize.y == 0:
            self._baseHeight = size.y

    def _checkExtents(self, endsExtent, minExtent):
        if endsExtent < 0:
            raise RuntimeError(
                    "Illegal value for endsExtent: %i. Must be >= 0"%endsExtent)
        elif endsExtent == 0:
            # 1 has same effect as 0 - we just create one-pixel wide start and end images.
            endsExtent = 1

        if minExtent == -1:
            minExtent = endsExtent*2+1
        else:
            minExtent = minExtent
        return (endsExtent, minExtent)

    def _renderImage(self, srcBmp, node, pos, size):
        canvas = player.createCanvas(id="stretch_canvas", size=size)
        img = avg.ImageNode(pos=pos, parent=canvas.getRootNode())
        img.setBitmap(srcBmp)
        canvas.render()
        node.setBitmap(canvas.screenshot())
        player.deleteCanvas("stretch_canvas")


class HStretchNode(StretchNodeBase):

    def __init__(self, endsExtent, minExtent=-1, **kwargs):
        super(HStretchNode, self).__init__(**kwargs)

        (self.__endsExtent, self.__minExtent) = self._checkExtents(endsExtent, minExtent)

        self.__startImg = avg.ImageNode(parent=self)
        self.__centerImg = avg.ImageNode(parent=self)
        self.__endImg = avg.ImageNode(parent=self)
       
        self._initNodes()
    
    def _positionNodes(self, newSize):
        newSize = avg.Point2D(max(self.__minExtent, newSize.x), newSize.y)

        self.__centerImg.x = self.__endsExtent
        self.__centerImg.width = newSize.x - self.__endsExtent*2
        self.__endImg.x = newSize.x - self.__endsExtent
        
        self._baseSize = newSize

    def _renderImages(self):
        height = self._bmp.getSize().y
        self._renderImage(self._bmp, self.__startImg, (0,0), (self.__endsExtent, height))
        self._renderImage(self._bmp, self.__centerImg, 
                (-self.__endsExtent,0), (1, height))
        endOffset = self._bmp.getSize().x - self.__endsExtent
        self._renderImage(self._bmp, self.__endImg, 
                (-endOffset,0), (self.__endsExtent, height))
    

class VStretchNode(StretchNodeBase):

    def __init__(self, endsExtent, minExtent=-1, **kwargs):
        super(VStretchNode, self).__init__(**kwargs)

        (self.__endsExtent, self.__minExtent) = self._checkExtents(endsExtent, minExtent)

        self.__startImg = avg.ImageNode(parent=self)
        self.__centerImg = avg.ImageNode(parent=self)
        self.__endImg = avg.ImageNode(parent=self)
        
        self._initNodes()
    
    def _positionNodes(self, newSize):
        newSize = avg.Point2D(newSize.x, max(self.__minExtent, newSize.y))

        self.__centerImg.y = self.__endsExtent
        self.__centerImg.height = newSize.y - self.__endsExtent*2
        self.__endImg.y = newSize.y - self.__endsExtent
        
        self._baseSize = newSize

    def _renderImages(self):
        width = self._bmp.getSize().x
        self._renderImage(self._bmp, self.__startImg, (0,0), (width, self.__endsExtent))
        self._renderImage(self._bmp, self.__centerImg, 
                (0,-self.__endsExtent), (width, 1))
        endOffset = self._bmp.getSize().x - self.__endsExtent
        self._renderImage(self._bmp, self.__endImg, (0,-endOffset), 
                (width, self.__endsExtent))


class HVStretchNode(StretchNodeBase):

    def __init__(self, endsExtent, minExtent=(-1,-1), **kwargs):
        super(HVStretchNode, self).__init__(**kwargs)

        (hEndsExtent, hMinExtent) = self._checkExtents(endsExtent[0], minExtent[0])
        (vEndsExtent, vMinExtent) = self._checkExtents(endsExtent[1], minExtent[1])
        self.__endsExtent = avg.Point2D(hEndsExtent, vEndsExtent)
        self.__minExtent = avg.Point2D(hMinExtent, vMinExtent)
        
        self.__createNodes()

        self._initNodes()
    
    def __calcNodePositions(self, newSize):
        xPosns = (0, self.__endsExtent[0], newSize.x-self.__endsExtent[0], newSize.x)
        yPosns = (0, self.__endsExtent[1], newSize.y-self.__endsExtent[1], newSize.y)

        self.__nodePosns = []
        for y in range(4):
            curRow = []
            for x in range(4):
                curRow.append(avg.Point2D(xPosns[x], yPosns[y]))
            self.__nodePosns.append(curRow)

    def __createNodes(self):
        self.__nodes = []
        for y in range(3):
            curRow = []
            for x in range(3):
                node = avg.ImageNode(parent=self)
                curRow.append(node)
            self.__nodes.append(curRow)
        
    def _positionNodes(self, newSize):
        newSize = avg.Point2D(
                max(self.__minExtent.x, newSize.x),
                max(self.__minExtent.y, newSize.y))
       
        self.__calcNodePositions(newSize)

        for y in range(3):
            for x in range(3):
                pos = self.__nodePosns[y][x]
                size = self.__nodePosns[y+1][x+1] - self.__nodePosns[y][x]
                node = self.__nodes[y][x]
                node.pos = pos
                node.size = size

        self._baseSize = newSize

    def _renderImages(self):
        bmpSize = self._bmp.getSize()
        xPosns = (0, self.__endsExtent[0], bmpSize.x-self.__endsExtent[0], bmpSize.x)
        yPosns = (0, self.__endsExtent[1], bmpSize.y-self.__endsExtent[1], bmpSize.y)
        for y in range(3):
            for x in range(3):
                node = self.__nodes[y][x]
                pos = avg.Point2D(xPosns[x], yPosns[y])
                size = avg.Point2D(xPosns[x+1], yPosns[y+1]) - pos
                if x == 1:
                    size.x = 1
                if y == 1:
                    size.y = 1
                self._renderImage(self._bmp, node, -pos, size)


class SwitchNode(avg.DivNode):

    def __init__(self, nodeMap=None, visibleid=None, parent=None, **kwargs):
        super(SwitchNode, self).__init__(**kwargs)
        self.registerInstance(self, parent)
  
        self.__nodeMap = nodeMap
        if nodeMap:
            self.setNodeMap(nodeMap)
        if visibleid:
            self.setVisibleID(visibleid)

    def setNodeMap(self, nodeMap):
        self.__nodeMap = nodeMap
        for node in self.__nodeMap.itervalues():
            if node:
                # Only insert child if it hasn't been inserted yet.
                try:
                    self.indexOf(node)
                except RuntimeError:
                    self.appendChild(node)
        if self.size != (0,0):
            size = self.size
        else:
            key = list(self.__nodeMap.keys())[0]
            size = self.__nodeMap[key].size
        self.setSize(size)

    def getVisibleID(self):
        return self.__visibleid

    def setVisibleID(self, visibleid):
        if not (visibleid in self.__nodeMap):
            raise RuntimeError("'%s' is not a registered id." % visibleid)
        self.__visibleid = visibleid
        for node in self.__nodeMap.itervalues():
            node.active = False
        self.__nodeMap[visibleid].active = True

    visibleid = property(getVisibleID, setVisibleID)

    def getWidth(self):
        return self.__baseWidth

    def setWidth(self, width):
        self.__baseWidth = width
        self.__setChildSizes()

    __baseWidth = avg.DivNode.width
    width = property(getWidth, setWidth)

    def getHeight(self):
        return self.__baseHeight

    def setHeight(self, height):
        self.__baseHeight = height
        self.__setChildSizes()

    __baseHeight = avg.DivNode.height
    height = property(getHeight, setHeight)

    def getSize(self):
        return self.__baseSize

    def setSize(self, size):
        self.__baseSize = size
        self.__setChildSizes()
        
    __baseSize = avg.DivNode.size
    size = property(getSize, setSize)   

    def __setChildSizes(self):
        if self.__nodeMap:
            for node in self.__nodeMap.itervalues():
                if node:
                    node.size = self.__baseSize
                    # Hack to support min. size in StretchNodes
                    self.__baseSize = node.size
