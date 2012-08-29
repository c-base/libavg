#!/usr/bin/env python
# -*- coding: utf-8 -*-

from libavg import avg, player

offset = None

def onMouseDown(event):
    global offset
    node = event.node
    offset = node.getRelPos((event.x, event.y))
    node.setEventCapture()

def onMouseMove(event):
    global offset
    node = event.node
    if offset != None:
        node.x = event.x-offset[0]
        node.y = event.y-offset[1]

def onMouseUp(event):
    global offset
    node = event.node
    if offset != None:
        node.releaseEventCapture()
        offset = None;

canvas = player.createMainCanvas(size=(640,480))
rootNode = canvas.getRootNode()
node = avg.WordsNode(pos=(10,10), font="arial", text="Hello World", parent=rootNode)
node.connectEventHandler(avg.Event.CURSOR_DOWN, avg.Event.MOUSE, node, onMouseDown)
node.connectEventHandler(avg.Event.CURSOR_MOTION, avg.Event.MOUSE, node, onMouseMove)
node.connectEventHandler(avg.Event.CURSOR_UP, avg.Event.MOUSE, node, onMouseUp)

player.play()

