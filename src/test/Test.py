#!/usr/bin/env python
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

from optparse import OptionParser, OptionValueError
import sys
import os
import platform

# Import the correct version of libavg. Since it should be possible to
# run the tests without installing libavg, we add the location of the 
# uninstalled libavg to the path.
# TODO: This is a mess. 
sys.path += ['../wrapper/.libs', '../python']
if platform.system() == 'Darwin':
    sys.path += ['../..']     # Location of libavg in a mac installation. 

if platform.system() == 'Windows':
    from libavg import avg    # Under windows, there is no uninstalled version.
else:    
    import avg

SrcDir = os.getenv("srcdir",".")
os.chdir(SrcDir)

from LoggerTest import *
from PluginTest import *
from PlayerTest import *
from VectorTest import *
from WordsTest import *
from AVTest import *
from DynamicsTest import *
from PythonTest import *


def runConsoleTest():
    Player = avg.Player.get()
    Player.loadFile("video.avg")

if os.getenv("AVG_CONSOLE_TEST"):
    runConsoleTest()
else:
    def setPow2(option, opt, value, parser):
        if value not in ('yes','no'):
            raise OptionValueError('argument must be "yes" or "no"')
        setUsePOW2Textures(value == 'yes')
    def setMode(option, opt, value, parser):
        try:
            ycbcrmode = {
                    'shader': avg.shader,
                    'apple': avg.apple,
                    'none': avg.none,
                    }[value]
        except KeyError:
            print value
            raise OptionValueError('ycbcrmode must be shader, apple or none')
        setYCbCrMode(ycbcrmode)
    def setPixBuf(option, opt, value, parser):
        if value not in ('yes','no'):
            raise OptionValueError('argument must be "yes" or "no"')
        setUsePixelBuffers(value == 'yes')

    parser = OptionParser("usage: %prog [options] [<suite> [testcase] [testcase] [...]]")
    parser.add_option("-b", "--bpp", dest = "bpp",
            type = "int",
            help = "set pixel depth")
    parser.add_option("-2", "--usepow2textures", dest = "usepow2textures",
            type = "string",
            action = 'callback', callback = setPow2,
            help = "Use power of 2 textures (yes, no)")
    parser.add_option("-y", "--ycbcrmode", dest = "ycbcrmode",
            type = "string",
            action = 'callback', callback = setMode,
            help = "YCbCrMode, must be shader, apple or none")
    parser.add_option("-p", "--usepixelbuffers", dest = "usepixelbuffers",
            type = "string",
            action = 'callback', callback = setPixBuf,
            help = "Use pixel buffers (yes, no)")
    parser.set_defaults(bpp = 24)
    options, args = parser.parse_args()

    availableSuites = {
            'logger': (loggerTestSuite,{}),
            'plugin': (pluginTestSuite,{}),
            'player': (playerTestSuite, {'bpp':options.bpp}),
            'vector': (vectorTestSuite, {}),
            'words': (wordsTestSuite, {}),
            'av': (AVTestSuite, {}),
            'dynamics': (dynamicsTestSuite, {}),
            'python': (pythonTestSuite, {}),
            }
    tests = []
    if len(args): # suite
        suiteName = args.pop(0)
        if suiteName not in availableSuites:
            parser.print_usage()
            print "ERROR: unknown test suite, known suites:"
            print ", ".join(availableSuites.keys())
            sys.exit(1)
        else:
            suitesToRun = [availableSuites[suiteName]]
        tests = args
    else:
        suitesToRun = availableSuites.values()

    suite = unittest.TestSuite()
    for s, args in suitesToRun:
        args.update({'tests':tests})
        suite.addTest(s(**args))

    Player = avg.Player.get()
    Log = avg.Logger.get()
    Log.setCategories(
            Log.APP |
            Log.WARNING |
#            Log.PROFILE |
#            Log.PROFILE_LATEFRAMES |
#            Log.CONFIG  |
#            Log.MEMORY  |
#            Log.BLTS    |
#            Log.EVENTS  |
#            Log.EVENTS2 |
    0)

    runner = unittest.TextTestRunner()
    rmBrokenDir()
    rc = runner.run(suite)
    
    if rc.wasSuccessful():
        sys.exit(0)
    else:
        sys.exit(1)

