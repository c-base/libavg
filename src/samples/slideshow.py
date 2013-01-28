#!/usr/bin/env python
# -*- coding: utf-8 -*-

# libavg - Media Playback Engine.
# Copyright (C) 2003-2013 Ulrich von Zadow
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
# Original author of this file: Thomas Schott <scotty@c-base.org>
#

"""
Image slideshow example which shows all images found in the current working directory.

Images are cross-faded and some random motion/scaling is applied while they're shown.
This example also shows how to use libavg's BitmapManager to asynchronously load images
in background.
"""

# TODO:
# once gameapp supports custom cmdline options, add options for:
#  * image directory
#  * show/transition intervals
#  * max. move distance
#  * sorted/shuffled show order (shuffled yet)

import os
from random import shuffle, randint
from libavg import avg, player, gameapp


IMAGE_EXTENSIONS = ['.jpg', '.jpeg', '.png', '.gif', '.bmp', '.tga', '.tif', '.tiff']
SHOW_INTERVAL   = 6000 # [ms]
TRANS_INTERVAL  = 2000 # [ms]
CHANGE_INTERVAL = SHOW_INTERVAL + TRANS_INTERVAL
ANIM_INTERVAL   = CHANGE_INTERVAL + TRANS_INTERVAL
ANIM_MAX_MOVE   = 200 # [px]

Logger = avg.Logger.get()
BitmapMgr = avg.BitmapManager.get()


def scaleMax(srcSize, maxSize):
    """
    Returns scrSize aspect correct scaled to fit right into maxSize.
    """
    aspect = srcSize.x / srcSize.y
    if aspect < maxSize.x / maxSize.y:
        return avg.Point2D(maxSize.y * aspect, maxSize.y)
    return avg.Point2D(maxSize.x, maxSize.x / aspect)

def scaleMin(srcSize, minSize):
    """
    Returns scrSize aspect correct scaled to completely fill minSize.
    """
    aspect = srcSize.x / srcSize.y
    if aspect < minSize.x / minSize.y:
        return avg.Point2D(minSize.x, minSize.x / aspect)
    return avg.Point2D(minSize.y * aspect, minSize.y)


class Slide(avg.ImageNode):
    HIDE_DONE = avg.Publisher.genMessageID()

    def __init__(self, parent=None, **kwargs):
        super(Slide, self).__init__(opacity=0.0, **kwargs)
        self.registerInstance(self, parent)
        self.publish(Slide.HIDE_DONE)

    def show(self):
        s = self.getMediaSize()
        assert s.x and s.y
        # initial size and position (scaled to screen size and centered)
        self.size = scaleMax(s, self.parent.size)
        self.pos = (self.parent.size - self.size) * 0.5
        # random final size and position (center moved by (dx, dy) and scaled up accordingly)
        dx = float(randint(-ANIM_MAX_MOVE, ANIM_MAX_MOVE))
        dy = float(randint(-ANIM_MAX_MOVE, ANIM_MAX_MOVE))
        size = scaleMin(s, self.size + avg.Point2D(abs(dx), abs(dy)) * 2.0)
        pos = self.pos + avg.Point2D(dx, dy) + (self.size - size) * 0.5
        # start in-transition
        avg.fadeIn(self, TRANS_INTERVAL)
        # start move/scale animation
        avg.ParallelAnim([
                avg.LinearAnim(self, 'size', ANIM_INTERVAL, self.size, size),
                avg.LinearAnim(self, 'pos', ANIM_INTERVAL, self.pos, pos)]).start()

    def hide(self):
        # start out-transition, notify subscribers when finished
        avg.fadeOut(self, TRANS_INTERVAL,
                lambda: self.notifySubscribers(Slide.HIDE_DONE, [self]))


class SlideshowApp(gameapp.GameApp):
    multitouch = False

    def init(self):
        imgDir = os.getcwd()
        if not os.path.isdir(imgDir):
            Logger.trace(Logger.ERROR, 'Directory [%s] not found' %imgDir)
            exit(1)
        Logger.trace(Logger.APP, 'Scanning directory [%s] ...' %imgDir)

        imgExts = tuple(IMAGE_EXTENSIONS + [ext.upper() for ext in IMAGE_EXTENSIONS])
        self.__imgFiles = [os.path.join(imgDir, imgFile) for imgFile in
                filter(lambda f: f.endswith(imgExts), os.listdir(imgDir))]
        if not self.__imgFiles:
            Logger.trace(Logger.ERROR, 'No image files found, '
                    'scanned file extensions:\n%s' %(', '.join(imgExts)))
            exit(1)
        l = len(self.__imgFiles)
        Logger.trace(Logger.APP, '%d image file%s found' %(l, 's' if l > 1 else ''))
        shuffle(self.__imgFiles)

        self.__slidesDiv = avg.DivNode(size=self._parentNode.size, parent=self._parentNode)
        # ping-pong two slides for cross-fade transition
        self.__newSlide = Slide(parent=self.__slidesDiv)
        self.__oldSlide = Slide(href=self.__imgFiles[0], parent=self.__slidesDiv)
        # HIDE_DONE notifications will trigger asynchronous pre-loading of the next image
        self.__newSlide.subscribe(Slide.HIDE_DONE, self.__asyncPreload)
        self.__oldSlide.subscribe(Slide.HIDE_DONE, self.__asyncPreload)
        self.__currentIdx = 0
        self.__changeSlide()
        player.setInterval(CHANGE_INTERVAL, self.__changeSlide)

    def __asyncPreload(self, slide):
        self.__currentIdx = (self.__currentIdx + 1) % len(self.__imgFiles)
        BitmapMgr.loadBitmap(self.__imgFiles[self.__currentIdx], slide.setBitmap)

    def __changeSlide(self):
        self.__oldSlide, self.__newSlide = self.__newSlide, self.__oldSlide
        self.__slidesDiv.reorderChild(self.__newSlide, 1) # move to top
        self.__newSlide.show()
        self.__oldSlide.hide()


if __name__ == '__main__':
    SlideshowApp.start()

