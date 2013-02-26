#!/usr/bin/env python
# -*- coding: utf-8 -*-
#
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
#

import sys
import optparse
from libavg import avg, AVGApp, player, widget

class VideoPlayer(AVGApp):

    CONTROL_WIDTH=240

    def __init__(self, parentNode):
        if options.fullscreen:
            player.setResolution(True, 1920, 1200, 0)
        AVGApp.__init__(self, parentNode)

    def init(self):
        self.node = avg.VideoNode(href=args[0], 
                accelerated=not(options.disableAccel))
        self.node.play()
        self.node.x = (self._parentNode.width-self.node.width)/2
        self.node.y = (self._parentNode.height-self.node.height)/2
       
        if self.node.hasAlpha():
            self.__makeAlphaBackground()
        self._parentNode.appendChild(self.node)
        self.curFrameWords = avg.WordsNode(parent=self._parentNode, pos=(10, 10), 
                fontsize=10)
        self.framesQueuedWords = avg.WordsNode(parent=self._parentNode, pos=(10, 22), 
                fontsize=10)

        controlPos = ((self._parentNode.width-VideoPlayer.CONTROL_WIDTH)/2, 
                self._parentNode.height-25)
        self.videoControl = widget.MediaControl(pos=controlPos, 
                size=(VideoPlayer.CONTROL_WIDTH, 20), 
                duration=self.node.getDuration(),
                parent=self._parentNode)
        self.videoControl.play()
        self.videoControl.subscribe(widget.MediaControl.PLAY_CLICKED, self.onPlay)
        self.videoControl.subscribe(widget.MediaControl.PAUSE_CLICKED, self.onPause)
        self.videoControl.subscribe(widget.MediaControl.SEEK_PRESSED, self.onSeekStart)
        self.videoControl.subscribe(widget.MediaControl.SEEK_RELEASED, self.onSeekEnd)
        self.videoControl.subscribe(widget.MediaControl.SEEK_MOTION, self.onSeek)

        player.subscribe(player.ON_FRAME, self.onFrame)
        self.isSeeking = False
    
    def onKeyDown(self, event):
        curTime = self.node.getCurTime()
        if event.keystring == "right":
            self.node.seekToTime(curTime+10000)
        elif event.keystring == "left":
            if curTime > 10000:
                self.node.seekToTime(curTime-10000)
            else:
                self.node.seekToTime(0)
        return False

    def onFrame(self):
        curFrame = self.node.getCurFrame()
        numFrames = self.node.getNumFrames()
        self.curFrameWords.text = "Frame: %i/%i"%(curFrame, numFrames)
        framesQueued = self.node.getNumFramesQueued()
        self.framesQueuedWords.text = "Frames queued: "+str(framesQueued)
        if not(self.isSeeking):
            self.videoControl.time = self.node.getCurTime()

    def onPlay(self):
        self.node.play()

    def onPause(self):
        self.node.pause()

    def onSeekStart(self):
        self.node.pause()
        self.isSeeking = True

    def onSeekEnd(self):
        self.node.play()
        self.isSeeking = False

    def onSeek(self, time):
        self.node.seekToTime(int(time))

    def __makeAlphaBackground(self):
        SQUARESIZE=40
        size = self.node.getMediaSize()
        avg.RectNode(parent=self._parentNode, size=self.node.getMediaSize(), 
                strokewidth=0, fillcolor="FFFFFF", fillopacity=1)
        for y in xrange(0, int(size.y)/SQUARESIZE):
            for x in xrange(0, int(size.x)/(SQUARESIZE*2)):
                pos = avg.Point2D(x*SQUARESIZE*2, y*SQUARESIZE)
                if y%2==1:
                    pos += (SQUARESIZE, 0)
                avg.RectNode(parent=self._parentNode, pos=pos,
                        size=(SQUARESIZE, SQUARESIZE), strokewidth=0, fillcolor="C0C0C0",
                        fillopacity=1)

parser = optparse.OptionParser("Usage: %prog <filename> [options]")
parser.add_option("-d", "--disable-accel", dest="disableAccel", action="store_true",
        default=False, help="disable vdpau acceleration")
parser.add_option("-f", "--fullscreen", dest="fullscreen", action="store_true",
        default=False)
(options, args) = parser.parse_args()

if len(args) == 0:
    parser.print_help()
    sys.exit(1)

argsNode = avg.VideoNode(href=args[0], loop=True, accelerated=False)
argsNode.pause()
size = argsNode.getMediaSize()
size = (max(size.x, 320), max(size.y, 120))
VideoPlayer.start(resolution=size)

