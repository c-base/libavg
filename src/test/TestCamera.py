#!/usr/bin/python
# -*- coding: utf-8 -*-

import avg

import unittest
import sys, syslog

class CameraTestCase(unittest.TestCase):
    def __init__(self, testFuncName, engine, bpp):
        self.__engine = engine
        self.__bpp = bpp;
        self.__testFuncName = testFuncName
        unittest.TestCase.__init__(self, testFuncName)
        print "-------- ", self.__testFuncName, " --------"
    def setUp(self):
        Player.setDisplayEngine(self.__engine)
        Player.setResolution(0, 0, 0, self.__bpp)
    def test(self):
        def setWhitebalance():
            self.__camera.whitebalance = 24407
        def resetWhitebalance():
            self.__camera.whitebalance = -1
        def changeBrightness():
            self.brightness += 10
            self.__camera.brightness = self.brightness
        def stopPlayback():
            self.__camera.stop()
            Player.setTimeout(200, self.__camera.play)
        self.curFrame = 200
        Player.loadFile("camera.avg")
        Player.setFramerate(60)
        self.__camera = Player.getElementByID("camera")
        self.__camera.play()
        self.brightness = 0
#        Player.setInterval(200, changeBrightness)
#        Player.setTimeout(200, setWhitebalance)
#        Player.setTimeout(300, resetWhitebalance)
#        Player.setInterval(500, stopPlayback)
        Player.play()

def playerTestSuite(engine, bpp):
    suite = unittest.TestSuite()
    suite.addTest(CameraTestCase("test", engine, bpp))
    return suite

Player = avg.Player()
Log = avg.Logger.get()
Log.setCategories(Log.APP |
          Log.WARNING | 
          Log.PROFILE |
#          Log.PROFILE_LATEFRAMES |
          Log.CONFIG |
          Log.MEMORY  |
#          Log.BLTS    |
          Log.EVENTS)

runner = unittest.TextTestRunner()

if len(sys.argv) != 3:
    print "Usage: TestCamera.py <display engine> <bpp>"
else:
    if sys.argv[1] == "OGL":
        engine = avg.OGL
    elif sys.argv[1] == "DFB":
        engine = avg.DFB
    else:
        print "First parameter must be OGL or DFB"
    bpp = int(sys.argv[2])
    runner.run(playerTestSuite(engine, bpp))

