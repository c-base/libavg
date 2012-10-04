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
from base import SwitchNode, StretchNode, Orientation
import gesture

import math


class ScrollBarTrack(SwitchNode):
    
    def __init__(self, enabledSrc, endsExtent, disabledSrc=None, 
            orientation=Orientation.HORIZONTAL, minExtent=-1, 
            **kwargs):
      
        super(ScrollBarTrack, self).__init__(nodeMap=None, **kwargs)
        self.__enabledNode = StretchNode(src=enabledSrc, endsExtent=endsExtent,
                orientation=orientation, minExtent=minExtent,
                parent=self)
        if disabledSrc == None:
            disabledSrc = enabledSrc
        self.__disabledNode = StretchNode(src=disabledSrc, endsExtent=endsExtent,
                orientation=orientation, minExtent=minExtent,
                parent=self)
        
        self.setNodeMap({
            "ENABLED": self.__enabledNode, 
            "DISABLED": self.__disabledNode
        })
        self.visibleid = "ENABLED"
        

class ScrollBarThumb(SwitchNode):
    
    def __init__(self, upSrc, downSrc, endsExtent, disabledSrc=None, 
            orientation=Orientation.HORIZONTAL, minExtent=-1, 
            **kwargs):
      
        super(ScrollBarThumb, self).__init__(nodeMap=None, **kwargs)
        self.__upNode = StretchNode(src=upSrc, endsExtent=endsExtent,
                orientation=orientation, minExtent=minExtent)
        self.__downNode = StretchNode(src=downSrc, endsExtent=endsExtent,
                orientation=orientation, minExtent=minExtent)
        if disabledSrc == None:
            disabledSrc = upSrc
        self.__disabledNode = StretchNode(src=disabledSrc, endsExtent=endsExtent,
                orientation=orientation, minExtent=minExtent)

        self.setNodeMap({
            "UP": self.__upNode, 
            "DOWN": self.__downNode, 
            "DISABLED": self.__disabledNode
        })
        self.visibleid = "UP"
        

class SliderThumb(SwitchNode):

    def __init__(self, upSrc, downSrc, disabledSrc=None, **kwargs):
        upNode = avg.ImageNode(href=upSrc)
        if disabledSrc == None:
            disabledSrc = upSrc
        nodeMap = {
            "UP": upNode,
            "DOWN": avg.ImageNode(href=downSrc),
            "DISABLED": avg.ImageNode(href=disabledSrc)
        }
        super(SliderThumb, self).__init__(nodeMap=nodeMap, visibleid="UP", **kwargs)


class Slider(avg.DivNode):

    THUMB_POS_CHANGED = avg.Publisher.genMessageID()
    PRESSED = avg.Publisher.genMessageID()
    RELEASED = avg.Publisher.genMessageID()

    def __init__(self, trackNode, thumbNode,
            enabled=True, orientation=Orientation.HORIZONTAL, range=(0.,1.), 
            thumbpos=0.0, parent=None, **kwargs):
        super(Slider, self).__init__(**kwargs)
        self.registerInstance(self, parent)
        
        self.publish(Slider.THUMB_POS_CHANGED)
        self.publish(Slider.PRESSED)
        self.publish(Slider.RELEASED)

        self._orientation = orientation

        self._trackNode = trackNode
        self.appendChild(self._trackNode)

        self._thumbNode = thumbNode
        self.appendChild(self._thumbNode)

        self._range = range
        self._thumbPos = thumbpos

        self._positionNodes()

        self.__recognizer = gesture.DragRecognizer(self._thumbNode, friction=-1,
                    detectedHandler=self.__onDragStart, moveHandler=self.__onDrag, 
                    upHandler=self.__onUp)

        if not(enabled):
            self.setEnabled(False)

    def getWidth(self):
        return self.__baseWidth

    def setWidth(self, width):
        self.__baseWidth = width
        self._positionNodes()

    __baseWidth = avg.DivNode.width
    width = property(getWidth, setWidth)

    def getHeight(self):
        return self.__baseHeight

    def setHeight(self, height):
        self.__baseHeight = height
        self._positionNodes()

    __baseHeight = avg.DivNode.height
    height = property(getHeight, setHeight)

    def getSize(self):
        return self.__baseSize

    def setSize(self, size):
        self.__baseSize = size
        self._positionNodes()

    __baseSize = avg.DivNode.size
    size = property(getSize, setSize)

    def getRange(self):
        return self._range

    def setRange(self, range):
        self._range = (float(range[0]), float(range[1]))
        self._positionNodes()

    # range[1] > range[0]: Reversed scrollbar.
    range = property(getRange, setRange)

    def getThumbPos(self):
        return self._thumbPos

    def setThumbPos(self, thumbpos):
        self._positionNodes(thumbpos)

    thumbpos = property(getThumbPos, setThumbPos)

    def getEnabled(self):
        return self._trackNode.visibleid != "DISABLED"

    def setEnabled(self, enabled):
        if enabled:
            if self._trackNode.visibleid == "DISABLED":
                self._trackNode.visibleid = "ENABLED"
                self._thumbNode.visibleid = "UP"
                self.__recognizer.enable(True)
        else:
            if self._trackNode.visibleid != "DISABLED":
                self._trackNode.visibleid = "DISABLED"
                self._thumbNode.visibleid = "DISABLED"
                self.__recognizer.enable(False)

    enabled = property(getEnabled, setEnabled)

    def __onDragStart(self):
        self._thumbNode.visibleid = "DOWN"
        self.__dragStartPos = self._thumbPos
        self.notifySubscribers(Slider.PRESSED, [])

    def __onDrag(self, offset):
        pixelRange = self._getScrollRangeInPixels()
        if pixelRange == 0:
            normalizedOffset = 0
        else:
            if self._orientation == Orientation.HORIZONTAL:
                normalizedOffset = offset.x/pixelRange
            else:
                normalizedOffset = offset.y/pixelRange
        self._positionNodes(self.__dragStartPos + normalizedOffset*self._getSliderRange())

    def __onUp(self, offset):
        self.__onDrag(offset)
        self._thumbNode.visibleid = "UP"
        self.notifySubscribers(Slider.RELEASED, [])

    def _getScrollRangeInPixels(self):
        if self._orientation == Orientation.HORIZONTAL:
            return self.size.x - self._thumbNode.size.x
        else:
            return self.size.y - self._thumbNode.size.y

    def _positionNodes(self, newSliderPos=None):
        oldThumbPos = self._thumbPos
        if newSliderPos is not None:
            self._thumbPos = float(newSliderPos)
        self._trackNode.size = self.size
                 
        self._constrainSliderPos()
        if self._thumbPos != oldThumbPos:
            self.notifySubscribers(Slider.THUMB_POS_CHANGED, [self._thumbPos])

        pixelRange = self._getScrollRangeInPixels()
        if self._getSliderRange() == 0:
            thumbPixelPos = 0
        else:
            thumbPixelPos = (((self._thumbPos-self._range[0])/self._getSliderRange())*
                    pixelRange)
        if self._orientation == Orientation.HORIZONTAL:
            self._thumbNode.x = thumbPixelPos
        else:
            self._thumbNode.y = thumbPixelPos

    def _getSliderRange(self):
        return self._range[1] - self._range[0]

    def _constrainSliderPos(self):
        rangeMin = min(self._range[0], self._range[1])
        rangeMax = max(self._range[0], self._range[1])
        self._thumbPos = max(rangeMin, self._thumbPos)
        self._thumbPos = min(rangeMax, self._thumbPos)


#class BmpSlider(Slider):
#
#    def __init__(self, trackSrc, trackEndsExtent, 
#            thumbUpSrc, thumbDownSrc, trackDisabledSrc=None, thumbDisabledSrc=None,
#            orientation=Orientation.HORIZONTAL, **kwargs):
#        trackNode = ScrollBarTrack(orientation=orientation, enabledSrc=trackSrc, 
#                disabledSrc=trackDisabledSrc, endsExtent=trackEndsExtent)
#        thumbNode = SliderThumb(upSrc=thumbUpSrc, 
#                downSrc=thumbDownSrc, disabledSrc=thumbDisabledSrc)
#
#        super(BmpSlider, self).__init__(trackNode=trackNode,
#                orientation=orientation, thumbNode=thumbNode, **kwargs)
    

class ScrollBar(Slider):
   
    def __init__(self, thumbextent=0.1, **kwargs):
        self.__thumbExtent = thumbextent
        super(ScrollBar, self).__init__(**kwargs)

    def getThumbExtent(self):
        return self.__thumbExtent

    def setThumbExtent(self, thumbExtent):
        self.__thumbExtent = float(thumbExtent)
        self._positionNodes()

    thumbextent = property(getThumbExtent, setThumbExtent)

    def _getScrollRangeInPixels(self):
        if self._orientation == Orientation.HORIZONTAL:
            return self.size.x - self._thumbNode.width
        else:
            return self.size.y - self._thumbNode.height

    def _positionNodes(self, newSliderPos=None):
        effectiveRange = math.fabs(self._range[1] - self._range[0])
        if self._orientation == Orientation.HORIZONTAL:
            thumbExtent = (self.__thumbExtent/effectiveRange)*self.size.x
            self._thumbNode.width = thumbExtent
        else:
            thumbExtent = (self.__thumbExtent/effectiveRange)*self.size.y
            self._thumbNode.height = thumbExtent
        super(ScrollBar, self)._positionNodes(newSliderPos)
        if self._range[1] < self._range[0]:
            # Reversed (upside-down) scrollbar
            if self._orientation == Orientation.HORIZONTAL:
                self._thumbNode.x -= thumbExtent
            else:
                self._thumbNode.y -= thumbExtent

    def _getSliderRange(self):
        if self._range[1] > self._range[0]:
            return self._range[1] - self._range[0] - self.__thumbExtent
        else:
            return self._range[1] - self._range[0] + self.__thumbExtent

    def _constrainSliderPos(self):
        rangeMin = min(self._range[0], self._range[1])
        rangeMax = max(self._range[0], self._range[1])
        self._thumbPos = max(rangeMin, self._thumbPos)
        self._thumbPos = min(rangeMax-self.__thumbExtent, self._thumbPos)


#class BmpScrollBar(ScrollBar):
#
#    def __init__(self, trackSrc, trackEndsExtent, thumbUpSrc, thumbDownSrc, 
#            thumbEndsExtent, thumbDisabledSrc=None, trackDisabledSrc=None, 
#            orientation=Orientation.HORIZONTAL, **kwargs):
#        trackNode = ScrollBarTrack(orientation=orientation, enabledSrc=trackSrc, 
#                disabledSrc=trackDisabledSrc, endsExtent=trackEndsExtent)
#        thumbNode = ScrollBarThumb(orientation=orientation, 
#                upSrc=thumbUpSrc, downSrc=thumbDownSrc, 
#                disabledSrc=thumbDisabledSrc, endsExtent=thumbEndsExtent)
#        
#        super(BmpScrollBar, self).__init__(trackNode=trackNode, 
#                orientation=orientation, thumbNode=thumbNode, **kwargs)

