# -*- coding: utf-8 -*-
# libavg - Media Playback Engine.
# Copyright (C) 2003-2008 Ulrich von Zadow
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

import unittest

import time
import math

from libavg import avg
from testcase import *

class PlayerTestCase(AVGTestCase):
    def __init__(self, testFuncName):
        AVGTestCase.__init__(self, testFuncName)

    def testPoint(self):
        def almostEqual(p1, p2):
            return (abs(p1.x-p2.x) < 0.0001 and abs(p1.y-p2.y) < 0.0001)

        def testHash():
            ptMap = {}
            ptMap[avg.Point2D(0,0)] = 0
            ptMap[avg.Point2D(1,0)] = 1
            ptMap[avg.Point2D(0,0)] = 2
            self.assert_(len(ptMap) == 2)
            self.assert_(ptMap[avg.Point2D(0,0)] == 2)

        pt = avg.Point2D()
        self.assert_(pt == avg.Point2D(0,0))
        pt = avg.Point2D(10, 10)
        self.assert_(pt[0]==pt.x)
        self.assert_(pt[1]==pt.y)
        self.assert_(pt == avg.Point2D(10, 10))
        self.assert_(pt == (10, 10))
        self.assert_(pt == avg.Point2D([10, 10]))
        self.assert_(pt != avg.Point2D(11, 10))
        self.assert_(str(pt) == "(10,10)")
        pt2 = eval(repr(pt))
        self.assert_(pt2 == pt)
        testHash()
        self.assert_(almostEqual(avg.Point2D(10,0).getNormalized(), avg.Point2D(1,0)))
        self.assert_(almostEqual(pt.getRotated(math.pi, (5,5)), avg.Point2D(0,0)))
        self.assert_(-pt == (-10, -10))
        self.assert_(pt-(10,0) == (0,10))
        self.assert_(pt+(10,0) == (20,10))
        self.assert_(pt*2 == (20,20))
        self.assert_(2*pt == (20,20))
        pt.x = 21
        pt.y = 23
        self.assert_(pt == avg.Point2D(21, 23))
        pt.x -= 11
        pt.y -= 13
        pt += avg.Point2D(10, 10)
        self.assert_(pt == avg.Point2D(20, 20))
        pt -= avg.Point2D(6, 6)
        self.assert_(pt == avg.Point2D(14, 14))
        self.assert_(pt != avg.Point2D(13, 13))
        pt = pt/2.
        self.assert_(pt == avg.Point2D(7, 7))
        pt = avg.Point2D((10, 10))
        self.assert_(pt == (10, 10))
        self.assert_(len(pt) == 2)
        self.assert_(pt[0]==pt.x)
        self.assert_(pt[1]==pt.y)
        self.assertException(lambda: pt[2])
        self.assert_(almostEqual(avg.Point2D(10,0), avg.Point2D.fromPolar(0,10)))
        self.assertException(avg.Point2D(0,0).getNormalized)
        self.assertException(lambda: avg.Point2D(0,))
        self.assertException(lambda: avg.Point2D(0,1,2))
        for point in ((10,0), (0,10), (-10,0), (0,-10)):
            pt = avg.Point2D(point)
            angle = pt.getAngle()
            norm = pt.getNorm()
            self.assert_(almostEqual(pt, avg.Point2D.fromPolar(angle,norm)))

    def testBasics(self):
        def getFramerate():
            framerate = Player.getEffectiveFramerate()
            self.assert_(framerate > 0)

        Player.showCursor(0)
        Player.showCursor(1)
        root = self.loadEmptyScene()
        node = Player.createNode("""<image id="test1" href="rgb24-65x65.png"/>""")
        root.appendChild(node)
        self.start((
                  getFramerate,
                  lambda: self.compareImage("testbasics", False), 
                  lambda: Player.setGamma(0.3, 0.3, 0.3),
                  lambda: Player.showCursor(0),
                  lambda: Player.showCursor(1),
                 ))

    def testFakeTime(self):
        def checkTime():
            self.assert_(Player.getFrameTime() == 50)
            self.assert_(Player.getFrameDuration() == 50)
            self.assert_(Player.getEffectiveFramerate() == 20)

        root = self.loadEmptyScene()
        Player.setFakeFPS(20)
        self.start((
                 checkTime,
                ))

    def testDivResize(self):
        def checkSize (w, h):
            self.assert_(node.width == w)
            self.assert_(node.height == h)
            self.assert_(node.size == (w,h))
        
        def setSize (size):
            node.size = size
        
        def setWidth (w):
            node.width = w
        
        def setHeight (h):
            node.height = h

        self.__initDefaultScene()
        node = Player.getElementByID('nestedavg')

        self.start((
                 lambda: checkSize(128, 32),
                 lambda: setSize((14,15)),
                 lambda: checkSize(14,15),
                 lambda: setWidth(23),
                 lambda: checkSize(23,15),
                 lambda: setHeight(22),
                 lambda: checkSize(23,22),
                ))

    def testRotate(self):
        def onOuterDown(Event):
            self.onOuterDownCalled = True
        
        def fakeRotate():
            Player.getElementByID("outer").angle += 0.1
            Player.getElementByID("inner").angle -= 0.1
        
        def testCoordConversions():
            innerNode = Player.getElementByID("inner")
            relPos = innerNode.getRelPos((90, 80))
            self.assert_(almostEqual(relPos, (10, 10)))
            outerNode = Player.getElementByID("outer")
            relPos = outerNode.getRelPos((90, 80))
            self.assert_(almostEqual(relPos[0], 12.332806394528092) and
                    almostEqual(relPos[1], 6.9211188716194592))
            absPos = outerNode.getAbsPos(relPos)
            self.assert_(almostEqual(absPos, (90, 80)))
            self.assert_(outerNode.getElementByPos((10, 10)) == innerNode)
            self.assert_(outerNode.getElementByPos((0, 10)) == outerNode)
            self.assert_(outerNode.getElementByPos((-10, -110)) == None)
        
        def sendEvent(type, x, y):
            Helper = Player.getTestHelper()
            if type == avg.CURSORUP:
                button = False
            else:
                button = True
            Helper.fakeMouseEvent(type, button, False, False,
                        x, y, 1)
        
        def disableCrop():
            Player.getElementByID("outer").crop = False
            Player.getElementByID("inner").crop = False
           
        self.__initDefaultRotateScene()
        Player.getElementByID("outer").setEventHandler(
                avg.CURSORDOWN, avg.MOUSE, onOuterDown) 
        self.onOuterDownCalled = False
        self.start((
                 lambda: self.compareImage("testRotate1", False),
                 testCoordConversions,
                 fakeRotate,
                 lambda: self.compareImage("testRotate1a", False),
                 lambda: sendEvent(avg.CURSORDOWN, 85, 70),
                 lambda: sendEvent(avg.CURSORUP, 85, 70),
                 lambda: self.assert_(not(self.onOuterDownCalled)),
                 lambda: sendEvent(avg.CURSORDOWN, 85, 75),
                 lambda: sendEvent(avg.CURSORUP, 85, 75),
                 lambda: self.assert_(self.onOuterDownCalled),
                 disableCrop,
                 lambda: self.compareImage("testRotate1b", False),
                ))

    def testRotate2(self):
        root = self.loadEmptyScene()
        
        div1 = avg.DivNode(pos=(80,0), size=(160,120), pivot=(0,0), angle=1.57, 
                parent=root)
        avg.ImageNode(size=(16,16), href="rgb24-65x65.png", parent=div1)
        div2 = avg.DivNode(pos=(40,0), size=(110,80), pivot=(0,0), angle=1.57,
                crop=True, parent=div1)
        avg.ImageNode(pos=(0,0), size=(16,16), href="rgb24-65x65.png", parent=div2)
        avg.ImageNode(pos=(30,-6), size=(16,16), href="rgb24-65x65.png", parent=div2)
        self.start([lambda: self.compareImage("testRotate2", False)])
        
    def testRotatePivot(self):
        def setPivot (pos):
            node.pivot = pos
        
        def addPivot (offset):
            node.pivot += offset
        
        root = self.loadEmptyScene()
        node = avg.DivNode(pos=(80,0), size=(160,120), pivot=(0,0), angle=1.57,
                crop=True, parent=root)
        div = avg.DivNode(pos=(40,-20), size=(160,120), pivot=(0,0), angle=0.79,
                crop=True, parent=node)
        avg.ImageNode(pos=(-10,-10), size=(128,128), href="rgb24-65x65.png", parent=div)
        avg.ImageNode(pos=(0,10), size=(32,32), href="rgb24-65x65.png", parent=node)
        self.start((
            lambda: self.compareImage("testRotatePivot1", False),
            lambda: setPivot((10, 10)),
            lambda: self.compareImage("testRotatePivot2", False),
            lambda: addPivot((-8, 0)),
            lambda: self.compareImage("testRotatePivot3", False),
           ))

    def testOutlines(self):
        root = self.__initDefaultRotateScene()
        root.elementoutlinecolor = "FFFFFF"
        Player.getElementByID("inner").width = 100000
        Player.getElementByID("inner").height = 100000
        self.start([lambda: self.compareImage("testOutlines", False)])

    def testError(self):
        self.initDefaultImageScene()
        Player.setTimeout(1, lambda: undefinedFunction)
        Player.setTimeout(50, Player.stop)
        try:
            Player.play()
        except NameError:
            self.assert_(1)
        else:
            self.assert_(0)

    def testExceptionInTimeout(self):
        def throwException():
            raise ZeroDivisionError
        
        try:
            self.initDefaultImageScene()
            self.start([throwException])
        except ZeroDivisionError:
            self.assert_(1)
        else:
            self.assert_(0)

    def testInvalidImageFilename(self):
        def activateNode():
            div.active = 1
        
        root = self.loadEmptyScene()
        div = avg.DivNode(active=False, parent=root)
        avg.ImageNode(href="filedoesntexist.png", parent=div)
        self.start([activateNode])

    def testInvalidVideoFilename(self):
        def tryplay():
            assertException(lambda: video.play())
        
        root = self.loadEmptyScene()
        video = avg.VideoNode(href="filedoesntexist.avi", parent=root)
        self.start((
                 lambda: tryplay,
                 lambda: video.stop()
                ))

    def testTimeouts(self):
        class TestException(Exception):
            pass
        
        def timeout1():
            Player.clearInterval(self.timeout1ID)
            Player.clearInterval(self.timeout2ID)
            self.timeout1called = True
        
        def timeout2():
            self.timeout2called = True
        
        def wait():
            pass
        
        def throwException():
            raise TestException

        def initException():
            self.timeout3ID = Player.setTimeout(0, throwException)
            
        def setupTimeouts():
            self.timeout1ID = Player.setTimeout(0, timeout1)
            self.timeout2ID = Player.setTimeout(1, timeout2)
            
        self.timeout1called = False
        self.timeout2called = False
        self.__exceptionThrown = False
        try:
            self.initDefaultImageScene()
            self.start((
                     setupTimeouts,
                     wait,
                     lambda: self.assert_(self.timeout1called),
                     lambda: self.assert_(not(self.timeout2called)),
                     lambda: initException(),
                     wait,
                     wait,
                     wait,
                     wait,
                     wait))
        except TestException:
            self.__exceptionThrown = True
            
        self.assert_(self.__exceptionThrown)
        Player.clearInterval(self.timeout3ID)

    def testPanoImage(self):
        def changeProperties():
            node = Player.getElementByID("pano")
            node.sensorheight=10
            node.sensorwidth=15
            node.focallength=25
        
        def loadImage():
            node = Player.getElementByID("pano")
            node.href = "rgb24-65x65.png"

        root = self.loadEmptyScene()
        avg.ImageNode(size=(320,240), href="rgb24-65x65.png", parent=root)
        avg.PanoImageNode(id="pano", size=(160,120), href="panoimage.png",
                sensorwidth=4.60, sensorheight=3.97, focallength=12, parent=root)
        avg.ImageNode(pos=(120,0), size=(40,40), href="rgb24-65x65.png", parent=root)
        self.start((
                 lambda: self.compareImage("testPanoImage", False),
                 lambda: time.sleep,
                 changeProperties,
                 loadImage
                ))

    def testAVGFile(self):
        Player.loadFile("image.avg")
        self.start((
                 lambda: self.compareImage("testAVGFile", False),
                ))
        self.assertException(lambda: Player.loadFile("filedoesntexist.avg"))

    def testBroken(self):
        def testBrokenString(string):
            self.assertException(lambda: Player.loadString(string))
        
        # This isn't xml
        testBrokenString("""
            xxx<avg width="400" height="300">
            </avg>
        """)
        # This isn't avg
        testBrokenString("""
            <bla>hallo
            </bla>""")
        testBrokenString("""
            <avg width="640" height="480" invalidattribute="bla">
            </avg>
        """)

    def testMove(self):
        def moveit():
            node = Player.getElementByID("nestedimg1")
            node.x += 50
            node.opacity -= 0.7
            node = Player.getElementByID("nestedavg")
            node.x += 50
        
        def checkRelPos():
            RelPos = Player.getElementByID("obscured").getRelPos((50,52))
            self.assert_(RelPos == (0, 0))
      
        self.__initDefaultScene()
        self.start((
                 lambda: self.compareImage("testMove1", False),
                 moveit,
                 checkRelPos
                ))

    def testCropImage(self):
        def moveTLCrop():
            node = Player.getElementByID("img")
            node.x = -20
            node.y = -20
        
        def moveBRCrop():
            node = Player.getElementByID("img")
            node.x = 60
            node.y = 40
        
        def moveTLNegative():
            node = Player.getElementByID("img")
            node.x = -60
            node.y = -50
        
        def moveBRGone():
            node = Player.getElementByID("img")
            node.x = 140
            node.y = 100
        
        def rotate():
            node = Player.getElementByID("img")
            node.x = 10
            node.y = 10
            Player.getElementByID("nestedavg").angle = 1.0
            Player.getElementByID("bkgd").angle = 1.0
        
        root = self.loadEmptyScene()
        avg.ImageNode(id="bkgd", href="crop_bkgd.png", parent=root)
        root.appendChild(
                Player.createNode("""
                  <div id="nestedavg" x="40" y="30" width="80" height="60" crop="True">
                    <div id="nestedavg2" crop="True">
                      <div id="nestedavg3" crop="True">
                        <image id="img" x="10" y="10" width="40" height="40" 
                                href="rgb24-64x64.png"/>
                      </div>
                    </div>
                  </div>
                """))
        self.start((
                 lambda: self.compareImage("testCropImage1", False),
                 moveTLCrop,
                 lambda: self.compareImage("testCropImage2", False),
                 moveBRCrop,
                 lambda: self.compareImage("testCropImage3", False),
                 moveTLNegative,
                 lambda: self.compareImage("testCropImage4", False),
                 moveBRGone,
                 lambda: self.compareImage("testCropImage5", False),

                 rotate,
                 lambda: self.compareImage("testCropImage6", False),
                 moveTLCrop,
                 lambda: self.compareImage("testCropImage7", False),
                 moveBRCrop,
                 lambda: self.compareImage("testCropImage8", False),
                 moveTLNegative,
                 lambda: self.compareImage("testCropImage9", False),
                 moveBRGone,
                 lambda: self.compareImage("testCropImage10", False)
               ))

    def testCropMovie(self):
        def playMovie():
            node = Player.getElementByID("movie")
            node.play()
        
        def moveTLCrop():
            node = Player.getElementByID("movie")
            node.x = -20
            node.y = -20
        
        def moveBRCrop():
            node = Player.getElementByID("movie")
            node.x = 60
            node.y = 40
        
        def moveTLNegative():
            node = Player.getElementByID("movie")
            node.x = -60
            node.y = -50
        
        def moveBRGone():
            node = Player.getElementByID("movie")
            node.x = 140
            node.y = 100
        
        def rotate():
            node = Player.getElementByID("movie")
            node.x = 10
            node.y = 10
            Player.getElementByID("nestedavg").angle = 1.0
            Player.getElementByID("bkgd").angle = 1.0
        
        Player.setFakeFPS(30)
        root = self.loadEmptyScene()
        avg.ImageNode(id="bkgd", href="crop_bkgd.png", parent=root)
        root.appendChild(
                Player.createNode("""
                  <div id="nestedavg" x="40" y="30" width="80" height="60" crop="True">
                    <video id="movie" x="10" y="10" width="40" height="40" 
                            threaded="false" href="../video/testfiles/mpeg1-48x48.mpg" 
                            fps="30"/>
                  </div>
                """))
        self.start((
                 playMovie,
                 lambda: self.compareImage("testCropMovie1", False),
                 moveTLCrop,
                 lambda: self.compareImage("testCropMovie2", False),
                 moveBRCrop,
                 lambda: self.compareImage("testCropMovie3", False),
                 moveTLNegative,
                 lambda: self.compareImage("testCropMovie4", False),
                 moveBRGone,
                 lambda: self.compareImage("testCropMovie5", False),

                 rotate,
                 lambda: self.compareImage("testCropMovie6", False),
                 moveTLCrop,
                 lambda: self.compareImage("testCropMovie7", False),
                 moveBRCrop,
                 lambda: self.compareImage("testCropMovie8", False),
                 moveTLNegative,
                 lambda: self.compareImage("testCropMovie9", False),
                 moveBRGone,
                 lambda: self.compareImage("testCropMovie10", False)
                ))

    def testWarp(self):
        def moveVertex():
            grid = image.getWarpedVertexCoords()
            grid[1][1] = (grid[1][1][0]+0.06, grid[1][1][1]+0.06)
            image.setWarpedVertexCoords(grid)
            grid = video.getWarpedVertexCoords()
            grid[0][0] = (grid[0][0][0]+0.06, grid[0][0][1]+0.06)
            grid[1][1] = (grid[1][1][0]-0.06, grid[1][1][1]-0.06)
            video.setWarpedVertexCoords(grid)
        
        def flip():
            grid = image.getOrigVertexCoords()
            grid = [ [ (1-pos[0], pos[1]) for pos in line ] for line in grid]
            image.setWarpedVertexCoords(grid)
       
        root = self.loadEmptyScene()
        image = avg.ImageNode(href="rgb24-64x64.png",
                maxtilewidth=32, maxtileheight=16, parent=root)
        video = avg.VideoNode(pos=(40,0), size=(80,80), opacity=0.5, loop=True,
                href="../video/testfiles/mpeg1-48x48.mpg", threaded=False, fps=30,
                parent=root)

        self.assertException(image.getOrigVertexCoords)
        self.assertException(image.getWarpedVertexCoords)
        Player.setFakeFPS(30)
        self.start((
                 lambda: video.play(),
                 lambda: self.compareImage("testWarp1", False),
                 moveVertex,
                 lambda: self.compareImage("testWarp2", False),
                 flip,
                 lambda: self.compareImage("testWarp3", False)
                ))

    def testMediaDir(self):
        def createImageNode():
            # Node is not in tree; mediadir should be root node dir.
            node = avg.ImageNode(href="rgb24-64x64a.png")
            self.assert_(node.size == avg.Point2D(0,0)) # File not found
            node.href = "rgb24-64x64.png"
            self.assert_(node.size == avg.Point2D(64,64)) # File found
            node = avg.ImageNode(href="rgb24-64x64a.png", width=23, height=42)
            # File not found, but custom size
            self.assert_(node.size == avg.Point2D(23,42))
            node.href = "rgb24-64x64.png"
            # File found, custom size stays
            self.assert_(node.size == avg.Point2D(23,42))
            node.size = (0,0)
            # File found, custom size cleared. Media size should be used.
            self.assert_(node.size == avg.Point2D(64,64))

        def setDir():
            div.mediadir="../video/testfiles"
        
        def setAbsDir():
            def absDir():
                # Should not find any media here...
                div.mediadir="/testmediadir"

            self.assertException(absDir)
        
        def createNode():
            node = avg.VideoNode(href="mjpeg1-48x48.avi", fps=30)

        root = self.loadEmptyScene()
        div = avg.DivNode(mediadir="testmediadir", parent=root)
        image = avg.ImageNode(pos=(0,30), href="rgb24-64x64a.png", parent=div)
        video = avg.VideoNode(href="mjpeg-48x48.avi", threaded=False, parent=div)
        self.start((
                 createImageNode,
                 lambda: video.play(),
                 lambda: self.compareImage("testMediaDir1", False),
                 setDir,
                 lambda: video.play(), 
                 lambda: self.compareImage("testMediaDir2", False),
                 lambda: self.assert_(image.width == 0),
                 createNode,
                 setAbsDir
                ))

    def testMemoryQuery(self):
        self.assert_(avg.getMemoryUsage() != 0)

    def testStopOnEscape(self):
        def pressEscape():
            Helper = Player.getTestHelper()
            escape = 27
            Helper.fakeKeyEvent(avg.KEYDOWN, escape, escape, "escape", escape, 
                    avg.KEYMOD_NONE),
            Helper.fakeKeyEvent(avg.KEYUP, escape, escape, "escape", escape, 
                    avg.KEYMOD_NONE),
        
        def testEscape1():
            Player.stopOnEscape(False)
            pressEscape()
        
        def testEscape2():
            Player.stopOnEscape(True)
            Player.stopOnEscape(False)
            pressEscape()
        
        def testEscape3():
            Player.stopOnEscape(True)
            pressEscape()
        
        def setAlive():
            self.testStopOnEscapeAlive = True

        self.testStopOnEscapeAlive = False
        self.__initDefaultScene()
        self.start((
                 testEscape1,
                 testEscape2,
                 setAlive
                ))
        self.assert_(self.testStopOnEscapeAlive)
        self.__initDefaultScene()
        self.start((
                 testEscape3, # this should exit the player
                 lambda: self.assert_(False),
                ))

    # Not executed due to bug #145 - hangs with some window managers.
    def testWindowFrame(self):
        def revertWindowFrame():
            Player.setWindowFrame(True)

        Player.setWindowFrame(False)
        self.__initDefaultScene()
        self.start([revertWindowFrame])

    def testScreenDimensions(self):
        res = Player.getScreenResolution()
        self.assert_(res.x > 0 and res.y > 0 and res.x < 10000 and res.y < 10000)
        ppmm = Player.getPixelsPerMM()
        self.assert_(ppmm > 0 and ppmm < 10000)
        mm = Player.getPhysicalScreenDimensions()
        self.assert_(mm.x > 0 and mm.y > 0 and mm.x < 10000 and mm.y < 10000)
#        print res, ppmm, mm
        Player.assumePhysicalScreenDimensions(mm)
        newPPMM = Player.getPixelsPerMM()
        newMM = Player.getPhysicalScreenDimensions()
        self.assert_(almostEqual(newPPMM, ppmm))
        self.assert_(newMM == mm)

    def __initDefaultScene(self):
        root = self.loadEmptyScene()
        avg.ImageNode(id="mainimg", size=(100, 75), href="rgb24-65x65.png", parent=root)
        div = avg.DivNode(id="nestedavg", pos=(0,32), opacity=1, size=(128, 32),
                crop=True, parent=root)
        avg.ImageNode(id="obscured", pos=(0,20), size=(96,40), href="rgb24-65x65.png",
                parent=div)
        avg.ImageNode(id="nestedimg1", size=(96,48), href="rgb24-65x65.png",
                parent=div)
        avg.ImageNode(id="nestedimg2", pos=(65,0), href="rgb24alpha-64x64.png",
                parent=div)

    def __initDefaultRotateScene(self):
        root = self.loadEmptyScene()
        div = avg.DivNode(pos=(80,10), size=(80,60), pivot=(0,0), angle=0.274,
                crop=True, parent=root)
        avg.ImageNode(pos=(10,10), size=(32,32), href="rgb24-65x65.png", parent=div)
        outerDiv = avg.DivNode(id="outer", pos=(80,70), size=(80,60),
                pivot=(0,0), angle=0.274, crop=True, parent=root)
        innerDiv = avg.DivNode(id="inner", size=(80,60), pivot=(0,0), angle=-0.274,
                crop=True, parent=outerDiv)
        avg.ImageNode(pos=(10,10), size=(32,32), href="rgb24-65x65.png", parent=innerDiv)
        return root

def playerTestSuite(tests):
    availableTests = (
            "testPoint",
            "testBasics",
            "testFakeTime",
            "testDivResize",
            "testRotate",
            "testRotate2",
            "testRotatePivot",
            "testOutlines",
            "testError",
            "testExceptionInTimeout",
            "testInvalidImageFilename",
            "testInvalidVideoFilename",
            "testTimeouts",
            "testPanoImage",
            "testAVGFile",
            "testBroken",
            "testMove",
            "testCropImage",
            "testCropMovie",
            "testWarp",
            "testMediaDir",
            "testMemoryQuery",
            "testStopOnEscape",
            "testScreenDimensions",
#            "testWindowFrame",
            )
    return createAVGTestSuite(availableTests, PlayerTestCase, tests)

Player = avg.Player.get()
