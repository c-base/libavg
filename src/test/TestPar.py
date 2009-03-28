#!/usr/bin/python
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

import sys
sys.path.append('/usr/local/lib/python2.3/site-packages/avg')
import avg
import time

class ParPortTestCase(unittest.TestCase):
    def test(self):
        def setAllLines(val):
            self.ParPort.setControlLine(avg.CONTROL_STROBE, val)
            self.ParPort.setControlLine(avg.CONTROL_AUTOFD, val)
            self.ParPort.setControlLine(avg.CONTROL_SELECT, val)
            self.ParPort.setControlLine(avg.CONTROL_INIT, val)
        self.ParPort = avg.ParPort()
        self.ParPort.init("")
        setAllLines(1)
        time.sleep(0.5)
        setAllLines(0)
        print self.ParPort.getStatusLine(avg.STATUS_ERROR)
        print self.ParPort.getStatusLine(avg.STATUS_SELECT)
        print self.ParPort.getStatusLine(avg.STATUS_PAPEROUT)
        print self.ParPort.getStatusLine(avg.STATUS_ACK)
        print self.ParPort.getStatusLine(avg.STATUS_BUSY)
        self.ParPort.setDataLines(avg.PARPORTDATA0 | avg.PARPORTDATA1)
        time.sleep(0.2)
        self.ParPort.setDataLines(avg.PARPORTDATA2 | avg.PARPORTDATA3)
        time.sleep(0.2)
        self.ParPort.clearDataLines(avg.PARPORTDATA2 | avg.PARPORTDATA3)
        time.sleep(0.2)
        self.ParPort.clearDataLines(avg.PARPORTDATA0 | avg.PARPORTDATA1)

def playerTestSuite():
    suite = unittest.TestSuite()
    suite.addTest(ParPortTestCase("test"))
    return suite

Player = avg.Player.get()

runner = unittest.TextTestRunner()
runner.run(playerTestSuite())

