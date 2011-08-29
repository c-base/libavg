#!/usr/bin/env python
# -*- coding: utf-8 -*-

from libavg import avg, ui
import libavg

RESOLUTION = avg.Point2D(800, 600)

def moveNodeToTop(node):
    parent = node.getParent()
    parent.reorderChild(node, parent.getNumChildren()-1)

def moveNodeOnScreen(node):
    center = node.pos + node.size/2
    if center.x < 0:
        node.pos = (-node.size.x/2, node.pos.y)
    if center.x > RESOLUTION.x:
        node.pos = (RESOLUTION.x-node.size.x/2, node.pos.y)
    if center.y < 0:
        node.pos = (node.pos.x, -node.size.y/2)
    if center.y > RESOLUTION.y:
        node.pos = (node.pos.x, RESOLUTION.y-node.size.y/2)


class TextRect(avg.DivNode):
    def __init__(self, text, color, bgcolor, **kwargs):
        avg.DivNode.__init__(self, size=(150,40), **kwargs)
        self.rect = avg.RectNode(size=self.size, fillopacity=1, fillcolor=bgcolor, 
                color=color, parent=self)
        self.words = avg.WordsNode(color=color, text=text, alignment="center", 
                parent=self)
        self.words.pos = (self.size-(0,self.words.size.y)) / 2

    def getSize(self):
        return self.__divSize

    def setSize(self, size):
        self.rect.size = size
        self.words.pos = (size-(0,self.words.size.y)) / 2
        self.__divSize = size
    __divSize = avg.DivNode.size
    size = property(getSize, setSize)


class TransformNode(TextRect):
    def __init__(self, text, ignoreScale, ignoreRotation, friction=-1, **kwargs):
        TextRect.__init__(self, text, "FFFFFF", "000000", **kwargs)

        self.transformer = ui.TransformRecognizer(
                node=self, 
                startHandler=self.__onStart,
                moveHandler=self.__onMove,
                upHandler=self.__onUp,
                ignoreScale=ignoreScale,
                ignoreRotation=ignoreRotation,
                friction=friction
                )

    def __onStart(self):
        self.baseTransform = ui.Mat3x3.fromNode(self)
        moveNodeToTop(self)

    def __onMove(self, transform):
        totalTransform = transform.applyMat(self.baseTransform)
        totalTransform.setNodeTransform(self)
        moveNodeOnScreen(self)

    def __onUp(self, transform):
        pass


class DragNode(TextRect):
    def __init__(self, text, friction=-1, **kwargs):
        TextRect.__init__(self, text, "FFFFFF", "000000", **kwargs)
    
        self.dragger = ui.DragRecognizer(
                eventNode=self,
                startHandler=self.__onStart,
                moveHandler=self.__onMove,
                upHandler=self.__onUp,
                stopHandler=self.__onStop,
                friction=friction
                )

    def __onStart(self, event):
        self.__dragStartPos = self.pos
        moveNodeToTop(self)

    def __onMove(self, event, offset):
        self.pos = self.__dragStartPos + offset
        moveNodeOnScreen(self)

    def __onUp(self, event, offset):
        self.pos = self.__dragStartPos + offset
        moveNodeOnScreen(self)

    def __onStop(self):
        pass


class GestureDemoApp(libavg.AVGApp):
    multitouch = True

    def init(self):
        TransformNode(text="TransformRecognizer",
                ignoreRotation=False,
                ignoreScale=False,
                pos=(20,20),
                parent=self._parentNode)

        TransformNode(text="TransformRecognizer<br/>ignoreRotation",
                ignoreRotation=True,
                ignoreScale=False,
                pos=(20,70),
                parent=self._parentNode)

        TransformNode(text="TransformRecognizer<br/>ignoreScale",
                ignoreRotation=False,
                ignoreScale=True,
                pos=(20,120),
                parent=self._parentNode)

        TransformNode(text="TransformRecognizer<br/>friction",
                ignoreRotation=False,
                ignoreScale=False,
                pos=(20,170),
                friction=0.02,
                parent=self._parentNode)

        DragNode(text="DragRecognizer",
                pos=(200,20),
                parent=self._parentNode)

        DragNode(text="DragRecognizer<br/>friction",
                pos=(200,70),
                friction=0.05,
                parent=self._parentNode)


if __name__ == '__main__':
    GestureDemoApp.start(resolution=RESOLUTION)
