#!/usr/bin/python
# -*- coding: utf-8 -*-
import unittest

import sys, syslog, time, os
sys.path = ['../python/.libs', '../python']
import avg
import anim

CREATE_BASELINE_IMAGES = False
BASELINE_DIR = "baseline"
RESULT_DIR = "resultimages"

ourSaveDifferences = True

class LoggerTestCase(unittest.TestCase):
    def test(self):
        self.Log = avg.Logger.get()
        self.Log.setCategories(self.Log.APP |
                  self.Log.WARNING | 
#                  self.Log.PROFILE |
#                  self.Log.PROFILE_LATEFRAMES |
                  self.Log.CONFIG |
                  self.Log.MEMORY | 
#                  self.Log.BLTS    |
                  self.Log.EVENTS
                  )
        try:
            os.remove("/tmp/testavg.log")
        except OSError:
            pass
        self.Log.setFileDest("/tmp/testavg.log")
        self.Log.trace(self.Log.APP, "Test file log entry.")
        stats = os.stat("/tmp/testavg.log")
        self.assert_(stats.st_size == 50)
        
        self.Log.setSyslogDest(syslog.LOG_USER, syslog.LOG_CONS)
        self.Log.trace(self.Log.APP, "Test syslog entry.")
        self.Log.setConsoleDest()

class NodeTestCase(unittest.TestCase):
    def testAttributes(self):
        self.Image = avg.Image()
        assert self.Image.id == ""
        self.Image.x = 10
        self.Image.x += 1
        assert self.Image.x == 11

class AVGTestCase(unittest.TestCase):
    def __init__(self, testFuncName, engine, bpp):
        self.__engine = engine
        self.__bpp = bpp
        self.__testFuncName = testFuncName
        self.Log = avg.Logger.get()
        unittest.TestCase.__init__(self, testFuncName)
    def setUp(self):
        Player.setDisplayEngine(self.__engine)
        Player.setResolution(0, 0, 0, self.__bpp)
        Player.setOGLOptions(UsePOW2Textures, YCbCrMode, UseRGBOrder, UsePixelBuffers, 1)
        print "-------- ", self.__testFuncName, " --------"
    def start(self, filename, actions):
        self.assert_(Player.isPlaying() == 0)
        Player.loadFile(filename)
        self.actions = actions
        self.curFrame = 0
        Player.setInterval(1, self.nextAction)
        Player.setFramerate(1000)
        Player.play()
        self.assert_(Player.isPlaying() == 0)
    def nextAction(self):
        self.actions[self.curFrame]()
        self.curFrame += 1
    def compareImage(self, fileName, warn):
        global CREATE_BASELINE_IMAGES
        Bmp = Player.screenshot()
        if CREATE_BASELINE_IMAGES:
            Bmp.save(BASELINE_DIR+"/"+fileName+".png")
        else:
            BaselineBmp = avg.Bitmap(BASELINE_DIR+"/"+fileName+".png")
            NumPixels = Player.getTestHelper().getNumDifferentPixels(Bmp, BaselineBmp)
            if (NumPixels > 20):
                if ourSaveDifferences:
                    Bmp.save(RESULT_DIR+"/"+fileName+".png")
                    BaselineBmp.save(RESULT_DIR+"/"+fileName+"_baseline.png")
                    Bmp.subtract(BaselineBmp)
                    Bmp.save(RESULT_DIR+"/"+fileName+"_diff.png")
                self.Log.trace(self.Log.WARNING, "Image compare: "+str(NumPixels)+
                        " bright pixels.")
                if warn:
                    self.Log.trace(self.Log.WARNING, "Image "+fileName
                            +" differs from original.")
                else:
                    self.assert_(False)

def keyUp():
    print "keyUp"

def keyDown():
    print "keyDown"
    Event = Player.getCurEvent()
    print Event
    print "  Type: "+str(Event.type)
    print "  keystring: "+Event.keystring
    print "  scancode: "+str(Event.scancode)
    print "  keycode: "+str(Event.keycode)
    print "  modifiers: "+str(Event.modifiers)

def dumpMouseEvent():
    Event = Player.getCurEvent()
    print Event
    print "  type: "+str(Event.type)
    print "  leftbuttonstate: "+str(Event.leftbuttonstate)
    print "  middlebuttonstate: "+str(Event.middlebuttonstate)
    print "  rightbuttonstate: "+str(Event.rightbuttonstate)
    print "  position: "+str(Event.x)+","+str(Event.y)
    print "  node: "+Event.node.id


def mainMouseUp():
    print "mainMouseUp"
#    dumpMouseEvent()

def mainMouseDown():
    print "mainMouseDown"
#    dumpMouseEvent()

def onMouseMove():
    print "onMouseMove"
#    dumpMouseEvent()

def onMouseUp():
    print "onMouseUp"
#    dumpMouseEvent()

def onMouseOver():
    print "onMouseOver"
    dumpMouseEvent()

def onMouseOut():
    print "onMouseOut"
    dumpMouseEvent()

def onMouseDown():
    print "onMouseDown"
    Player.getElementByID("mouseover1").active=0
    Player.getElementByID("rightdiv").active=0
    dumpMouseEvent()

def onErrMouseOver():
    undefinedFunction()

class PlayerTestCase(AVGTestCase):
    def __init__(self, testFuncName, engine, bpp):
        AVGTestCase.__init__(self, testFuncName, engine, bpp)
    def testImage(self):    
        def loadNewFile():
            Player.getElementByID("test").href = "rgb24alpha-64x64.png"
            Player.getElementByID("testhue").href = "rgb24alpha-64x64.png"
        def getBitmap():
            node = Player.getElementByID("test")
            Bmp = node.getBitmap()
            self.assert_(Bmp.getSize() == (65,65))
            self.assert_(Bmp.getFormat() == avg.R8G8B8 or Bmp.getFormat() == avg.B8G8R8)
        self.start("image.avg",
                (lambda: self.compareImage("testimg", False), 
                 getBitmap,
                 loadNewFile, 
                 lambda: self.compareImage("testimgload", False),
                 lambda: Player.setGamma(0.7, 0.7, 0.7),
                 lambda: Player.setGamma(1.0, 1.0, 1.0),
                 Player.stop))
    def testError(self):
        Player.loadFile("image.avg")
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
            self.start("image.avg",
                    (throwException,
                     Player.stop))
        except ZeroDivisionError:
            self.assert_(1)
        else:
            self.assert_(0)
    def testInvalidImageFilename(self):
        def activateNode():
            Player.getElementByID("enclosingdiv").active = 1
        self.start("invalidfilename.avg",
                (activateNode,
                 Player.stop))
    def testEvents(self):
        def getMouseState():
            Event = Player.getMouseState()
            Event.x
        self.start("events.avg", 
                (getMouseState, 
                 lambda: self.compareImage("testEvents", False), 
                 Player.stop))
    def testEventErr(self):
        Player.loadFile("errevent.avg")
        Player.setTimeout(10, Player.stop)
        try:
            Player.play()
        except NameError:
            print("(Intentional) NameError caught")
            self.assert_(1)
    def testHugeImage(self):
        def moveImage():
            Player.getElementByID("mainimg").x -= 2500
        self.start("hugeimage.avg",
                (lambda: self.compareImage("testHugeImage0", False),
                 moveImage,
                 lambda: self.compareImage("testHugeImage1", False),
                 Player.stop))
    def testPanoImage(self):
        def changeProperties():
            node = Player.getElementByID("pano")
            node.sensorheight=10
            node.sensorwidth=15
            node.focallength=25
        def loadImage():
            node = Player.getElementByID("pano")
            node.href = "rgb24-65x65.png"
        self.start("panoimage.avg",
                (lambda: self.compareImage("testPanoImage", False),
                 lambda: time.sleep,
                 changeProperties,
                 loadImage,
                 Player.stop))
    def testBroken(self):
        Player.loadFile("noxml.avg")
        Player.loadFile("noavg.avg")
        Player.loadFile("noavg2.avg")
    def testMove(self):
        def moveit():
            node = Player.getElementByID("nestedimg1")
            node.x += 50
            node.opacity -= 0.7
            node = Player.getElementByID("nestedavg")
            node.x += 50
        self.start("avg.avg",
                (lambda: self.compareImage("testMove1", False),
                 moveit,
                 lambda: self.compareImage("testMove2", False),
                 Player.stop))
    def testBlend(self):
        self.start("blend.avg",
                (lambda: self.compareImage("testBlend", False),
                 Player.stop))
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
        self.start("crop2.avg",
                (lambda: self.compareImage("testCropImage1", False),
                 moveTLCrop,
                 lambda: self.compareImage("testCropImage2", False),
                 moveBRCrop,
                 lambda: self.compareImage("testCropImage3", False),
                 moveTLNegative,
                 lambda: self.compareImage("testCropImage4", False),
                 moveBRGone,
                 lambda: self.compareImage("testCropImage5", False),
                 Player.stop))
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
        self.start("crop.avg",
                (playMovie,
                 lambda: self.compareImage("testCropMovie1", False),
                 moveTLCrop,
                 lambda: self.compareImage("testCropMovie2", False),
                 moveBRCrop,
                 lambda: self.compareImage("testCropMovie3", False),
                 moveTLNegative,
                 lambda: self.compareImage("testCropMovie4", False),
                 moveBRGone,
                 lambda: self.compareImage("testCropMovie5", False),
                 Player.stop))
    def testWarp(self):
        def moveVertex():
            node = Player.getElementByID("testtiles")
            pos = node.getWarpedVertexCoord(1,1)
            pos.x += 0.06
            pos.y += 0.06
            node.setWarpedVertexCoord(1,1,pos)
            node = Player.getElementByID("clogo1")
            pos = node.getWarpedVertexCoord(0,0)
            pos.x += 0.06
            pos.y += 0.06
            node.setWarpedVertexCoord(0,0,pos)
        def flip():
            node = Player.getElementByID("testtiles")
            for y in range(node.getNumVerticesY()):
                for x in range(node.getNumVerticesX()):
                    pos = node.getOrigVertexCoord(x,y)
                    pos.x = 1-pos.x
                    node.setWarpedVertexCoord(x,y,pos)
        self.start("video.avg",
                (lambda: Player.getElementByID("clogo1").play(),
                 lambda: self.compareImage("testWarp1", False),
                 moveVertex,
                 lambda: self.compareImage("testWarp2", True),
                 flip,
                 lambda: self.compareImage("testWarp3", False),
                 Player.stop))
    def testWords(self):
        def changeText():
            node = Player.getElementByID("cbasetext")
            str = "blue"
            node.text = str
            node.color = "404080"
            node.x += 10
        def changeHeight():
            node = Player.getElementByID("cbasetext")
            node.height = 28
        def activateText():
            Player.getElementByID('cbasetext').active = 1
        def deactivateText():
            Player.getElementByID('cbasetext').active = 0
        def changeFont():
            node = Player.getElementByID("cbasetext")
            node.font = "Times New Roman"
            node.size = 30
        def changeFont2():
            node = Player.getElementByID("cbasetext")
            node.size = 18
        def changeUnicodeText():
            Player.getElementByID("dynamictext").text = "Arabic nonsense: ﯿﭗ"
        self.start("text.avg",
                (lambda: self.compareImage("testWords1", True),
                 changeText,
                 changeHeight,
                 deactivateText,
                 lambda: self.compareImage("testWords2", True),
                 activateText,
                 changeFont,
                 lambda: self.compareImage("testWords3", True),
                 changeFont2,
                 changeUnicodeText,
                 lambda: self.compareImage("testWords4", True),
                 Player.stop))
    def testVideo(self):
        def seek():
            Player.getElementByID("clogo2").seekToFrame(180)
        def foo():
            pass
        def newHRef():
            node = Player.getElementByID("clogo2")
            node.href = "h264-48x48.h264"
            node.play()
        def move():
            node = Player.getElementByID("clogo2")
            node.x += 30
        def activateclogo():
            Player.getElementByID('clogo').active=1
        def deactivateclogo():
            Player.getElementByID('clogo').active=0
        self.start("video.avg",
                (lambda: self.compareImage("testVideo1", False),
                 lambda: Player.getElementByID("clogo2").play(),
                 seek,
                 lambda: self.compareImage("testVideo2", False),
                 lambda: Player.getElementByID("clogo2").pause(),
                 lambda: self.compareImage("testVideo3", False),
                 lambda: Player.getElementByID("clogo2").play(),
                 lambda: self.compareImage("testVideo4", False),
                 newHRef,
                 lambda: Player.getElementByID("clogo1").play(),
                 lambda: self.compareImage("testVideo5", False),
                 move,
                 lambda: Player.getElementByID("clogo").pause(),
                 lambda: self.compareImage("testVideo6", False),
                 deactivateclogo,
                 lambda: self.compareImage("testVideo7", False),
                 activateclogo,
                 lambda: self.compareImage("testVideo8", False),
                 lambda: Player.getElementByID("clogo").stop(),
                 lambda: self.compareImage("testVideo9", False),
                 Player.stop))
    def testAnim(self):
        def startAnim():
            def onStop():
                self.__animStopped = 1
            self.compareImage("testAnim1", False)
            anim.fadeOut(Player.getElementByID("nestedimg2"), 200)
            Player.getElementByID("nestedimg1").opacity = 0
            anim.fadeIn(Player.getElementByID("nestedimg1"), 200, 1)
            anim.LinearAnim(Player.getElementByID("nestedimg1"), "x", 
                    200, 0, 100, 0, onStop)
        def startSplineAnim():
            self.assert_(self.__animStopped == 1)
            self.compareImage("testAnim2", False)
            anim.SplineAnim(Player.getElementByID("mainimg"), "x", 
                    200, 100, -400, 10, 0, 0, None)
            anim.SplineAnim(Player.getElementByID("mainimg"), "y", 
                    200, 100, 0, 10, -400, 1, None)
        anim.init(Player)
        Player.loadFile("avg.avg")
        Player.setTimeout(10, startAnim)
        Player.setTimeout(390, startSplineAnim)
        Player.setTimeout(800, lambda: self.compareImage("testAnim3", False))
        Player.setTimeout(850, Player.stop)
        Player.setVBlankFramerate(1)
        Player.play()
        
#    def createNodes(self):
#        node=Player.createNode("<image href='rgb24-64x64.png'/>")
#        node.x = 10
#        node.y = 20
#        node.z = 2
#        node.opacity = 0.333
#        node.angle = 0.1
#        node.blendmode = "add"
##        print node.toXML()
#        self.rootNode.addChild(node)
##        nodeCopy = node
##        self.rootNode.addChild(nodeCopy)
#        node = Player.createNode("<video href='mpeg1-48x48.mpg'/>")
#        self.rootNode.addChild(node)
#        node = Player.createNode("<words text='Lorem ipsum dolor'/>")
#        self.rootNode.addChild(node)
#        node.size = 18
#        node.font = "times new roman"
#        node.parawidth = 200
#        node = Player.createNode("<div><image href='rgb24-64x64.png'/></div>")
#        node.getChild(0).x=10
#        node.x=10
#        self.rootNode.addChild(node)
#    def deleteNodes(self):
#        for i in range(self.rootNode.getNumChildren()-1,0):
##            print ("Deleting node #"+i);
#            self.rootNode.removeChild(i)
#    def testDynamics(self):
#        Player.loadFile("image.avg")
#        self.rootNode = Player.getRootNode()
#        print self.rootNode.indexOf(Player.getElementByID("mainimg"));
#        print self.rootNode.indexOf(Player.getElementByID("testtiles"));
#        self.createNodes()
#        Player.setTimeout(250, self.deleteNodes)
#        Player.setTimeout(500, self.createNodes)
#        Player.play()
   
            
def playerTestSuite(engine, bpp):
    def rmBrokenDir():
        try:
            files = os.listdir(RESULT_DIR)
            for file in files:
                os.remove(RESULT_DIR+"/"+file)
        except OSError:
            try:
                os.mkdir(RESULT_DIR)
            except OSError:
                # This can happen on make distcheck - permission denied...
                global ourSaveDifferences
                ourSaveDifferences = False
    rmBrokenDir()
    suite = unittest.TestSuite()
    suite.addTest(NodeTestCase("testAttributes"))
    suite.addTest(PlayerTestCase("testImage", engine, bpp))
    suite.addTest(PlayerTestCase("testError", engine, bpp))
    suite.addTest(PlayerTestCase("testExceptionInTimeout", engine, bpp))
    suite.addTest(PlayerTestCase("testInvalidImageFilename", engine, bpp))
    suite.addTest(PlayerTestCase("testEvents", engine, bpp))
    suite.addTest(PlayerTestCase("testEventErr", engine, bpp))
    suite.addTest(PlayerTestCase("testHugeImage", engine, bpp))
    suite.addTest(PlayerTestCase("testPanoImage", engine, bpp))
    suite.addTest(PlayerTestCase("testBroken", engine, bpp))
    suite.addTest(PlayerTestCase("testMove", engine, bpp))
    suite.addTest(PlayerTestCase("testBlend", engine, bpp))
    suite.addTest(PlayerTestCase("testCropImage", engine, bpp))
    suite.addTest(PlayerTestCase("testCropMovie", engine, bpp))
    suite.addTest(PlayerTestCase("testWarp", engine, bpp))
    suite.addTest(PlayerTestCase("testWords", engine, bpp))
    suite.addTest(PlayerTestCase("testVideo", engine, bpp))
    suite.addTest(PlayerTestCase("testAnim", engine, bpp))
    return suite

def completeTestSuite(engine, bpp):
    suite = unittest.TestSuite()
    suite.addTest(LoggerTestCase("test"))
    suite.addTest(playerTestSuite(engine, bpp))
    return suite

def getBoolParam(paramIndex):
    param = sys.argv[paramIndex].upper()
    if param == "TRUE":
        return True
    elif param == "FALSE":
        return False
    else:
        print "Parameter "+paramIndex+" must be 'True' or 'False'"

if len(sys.argv) == 1:
    engine = avg.OGL
    bpp = 24
    customOGLOptions = False
elif len(sys.argv) == 3 or len(sys.argv) == 7:
    if sys.argv[1] == "OGL":
        engine = avg.OGL
    elif sys.argv[1] == "DFB":
        engine = avg.DFB
    else:
        print "First parameter must be OGL or DFB"
    bpp = int(sys.argv[2])
    if (len(sys.argv) == 7):
        customOGLOptions = True
        UsePOW2Textures = getBoolParam(3)
        s = sys.argv[4]
        if s == "shader":
            YCbCrMode = avg.shader
        elif s == "apple":
            YCbCrMode = avg.apple
        elif s == "mesa":
            YCbCrMode = avg.mesa
        elif s == "none":
            YCbCrMode = avg.none
        else:
            print "Fourth parameter must be shader, apple, mesa or none"
        UseRGBOrder = getBoolParam(5)
        UsePixelBuffers = getBoolParam(6)
    else:
        customOGLOptions = False
else:
    print "Usage: Test.py [<display engine> <bpp>"
    print "               [<UsePOW2Textures> <YCbCrMode> <UseRGBOrder> <UsePixelBuffers>]]"
    sys.exit(1)

if not(customOGLOptions): 
    UsePOW2Textures = False 
    YCbCrMode = avg.shader
    UseRGBOrder = False 
    UsePixelBuffers = True

SrcDir = os.getenv("srcdir",".")
os.chdir(SrcDir)
print(os.getcwd())
os.system("whoami")
Player = avg.Player()
runner = unittest.TextTestRunner()
rc = runner.run(completeTestSuite(engine, bpp))
if rc.wasSuccessful():
    sys.exit(0)
else:
    sys.exit(1)

