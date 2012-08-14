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
#

from libavg import avg, textarea, ui, player
from libavg.ui import simple

from testcase import *

class UITestCase(AVGTestCase):
    def __init__(self, testFuncName):
        AVGTestCase.__init__(self, testFuncName)

    def testKeyboard(self):
        def setup():
            keyDefs = [
                    [("a", "A"),True, False, ( 5, 5), (30, 30)],
                    [(1, ),     True, False, (35, 5), (30, 30)],
                    ["SHIFT",   False, False, (65, 5), (50, 30)]]
            kbNoShift = ui.Keyboard("keyboard_bg.png", "keyboard_ovl.png", keyDefs, None,
                    pos=(10, 10), parent = root)
            kbNoShift.setKeyHandler(onKeyDown, onKeyUp)
            kbShift = ui.Keyboard("keyboard_bg.png", "keyboard_ovl.png", keyDefs, "SHIFT",
                    pos=(10, 60), selHref="keyboard_sel.png", parent = root)
            kbShift.setKeyHandler(onKeyDown, onKeyUp)

        def onKeyDown(event, char, cmd):
            self.__keyDown = True
            self.__keyUp   = False
            self.__char = char
            self.__cmd = cmd

        def onKeyUp(event, char, cmd):
            self.__keyDown = False
            self.__keyUp   = True
            self.__char = char
            self.__cmd = cmd

        root = self.loadEmptyScene()

        self.__keyDown = False
        self.__keyUp   = True
        self.__char = "foo"
        self.__cmd = "bar"
        self.start(False,
                (setup,
                 lambda: self.compareImage("testUIKeyboard"),
                 # test character key
                 lambda: self._sendMouseEvent(avg.CURSORDOWN, 30, 30),
                 lambda: self.assert_(self.__keyDown and not self.__keyUp),
                 lambda: self.assert_(self.__char == "a" and self.__cmd is None),
                 lambda: self.compareImage("testUIKeyboardDownA1"),
                 lambda: self._sendMouseEvent(avg.CURSORUP, 30, 30),
                 lambda: self.assert_(not self.__keyDown and self.__keyUp),
                 lambda: self.assert_(self.__char == "a" and self.__cmd is None),
                 lambda: self.compareImage("testUIKeyboard"),
                 # test command key
                 lambda: self._sendMouseEvent(avg.CURSORDOWN, 100, 30),
                 lambda: self.assert_(self.__keyDown and not self.__keyUp),
                 lambda: self.assert_(self.__char is None and self.__cmd == "SHIFT"),
                 lambda: self.compareImage("testUIKeyboardDownS1"),
                 lambda: self._sendMouseEvent(avg.CURSORUP, 100, 30),
                 lambda: self.assert_(not self.__keyDown and self.__keyUp),
                 lambda: self.assert_(self.__char is None and self.__cmd == "SHIFT"),
                 lambda: self.compareImage("testUIKeyboard"),
                 # test shift key (no shift key support)
                 lambda: self._sendTouchEvent(1, avg.CURSORDOWN, 100, 30),
                 lambda: self._sendTouchEvent(2, avg.CURSORDOWN, 30, 30),
                 lambda: self.assert_(self.__char == "a" and self.__cmd is None),
                 lambda: self._sendTouchEvent(3, avg.CURSORDOWN, 60, 30),
                 lambda: self.assert_(self.__char == 1 and self.__cmd is None),
                 lambda: self.compareImage("testUIKeyboardDownA111S1"),
                 lambda: self._sendTouchEvent(2, avg.CURSORUP, 30, 30),
                 lambda: self.assert_(self.__char == "a" and self.__cmd is None),
                 lambda: self._sendTouchEvent(3, avg.CURSORUP, 60, 30),
                 lambda: self.assert_(self.__char == 1 and self.__cmd is None),
                 lambda: self._sendTouchEvent(1, avg.CURSORUP, 100, 30),
                 lambda: self.compareImage("testUIKeyboard"),
                 # test shift key (with shift key support)
                 lambda: self._sendTouchEvent(1, avg.CURSORDOWN, 100, 80),
                 lambda: self._sendTouchEvent(2, avg.CURSORDOWN, 30, 80),
                 lambda: self.assert_(self.__char == "A" and self.__cmd is None),
                 lambda: self._sendTouchEvent(3, avg.CURSORDOWN, 60, 80),
                 lambda: self.assert_(self.__char == 1 and self.__cmd is None),
                 lambda: self.compareImage("testUIKeyboardDownA212S2"),
                 lambda: self._sendTouchEvent(2, avg.CURSORUP, 30, 80),
                 lambda: self.assert_(self.__char == "A" and self.__cmd is None),
                 lambda: self._sendTouchEvent(3, avg.CURSORUP, 60, 80),
                 lambda: self.assert_(self.__char == 1 and self.__cmd is None),
                 lambda: self._sendTouchEvent(1, avg.CURSORUP, 100, 80),
                 lambda: self.compareImage("testUIKeyboard"),
                 # test drag over keys 
                 lambda: self._sendTouchEvent(1, avg.CURSORDOWN, 60, 80),
                 lambda: self.assert_(self.__char == 1 and self.__cmd is None),
                 lambda: self.compareImage("testUIKeyboardDown11"),
                 lambda: self._sendTouchEvent(1, avg.CURSORMOTION, 60, 50),
                 lambda: self.assert_(self.__char == 1 and self.__cmd is None),
                 lambda: self.compareImage("testUIKeyboard"),
                 lambda: self._sendTouchEvent(1, avg.CURSORMOTION, 100, 80),
                 lambda: self.assert_(self.__char is None and self.__cmd == "SHIFT"),
                 lambda: self.compareImage("testUIKeyboardDownA2S1"),
                 lambda: self._sendTouchEvent(1, avg.CURSORMOTION, 60, 80),
                 lambda: self.compareImage("testUIKeyboardDown11"),
                 lambda: self._sendTouchEvent(1, avg.CURSORUP, 60, 80),
                 lambda: self.assert_(not self.__keyDown and self.__keyUp),
                ))

    def testTextArea(self):
        def setup():
            self.ta1 = textarea.TextArea( pos=(2,2), size=(156, 96), parent=root)
            self.ta1.setStyle(font='Bitstream Vera Sans', variant='Roman',
                fontsize=16, multiline=True, color='FFFFFF')
            self.ta1.setText('Lorem ipsum')
            self.ta1.setFocus(True) # TODO: REMOVE

            self.ta2 = textarea.TextArea(pos=(2,100), size=(156, 18), parent=root)
            self.ta2.setStyle(font='Bitstream Vera Sans', variant='Roman',
                fontsize=14, multiline=False, color='4b94ef', cursorColor='FF0000', 
                flashingCursor=False)
            self.ta2.setText('sit dolor')
            self.ta2.showCursor(False)
            self.ta2.setFocus(True) # TODO: REMOVE

        def setAndCheck(ta, text):
            ta.setText(text)
            self.assertEqual(ta.getText(), text)

        def clear(ta):
            ta.onKeyDown(textarea.KEYCODE_FORMFEED)
            self.assertEqual(ta.getText(), '')

        def testUnicode():
            self.ta1.setText(u'some ùnìcöde')
            self.ta1.onKeyDown(textarea.KEYCODES_BACKSPACE[0])
            self.assertEqual(self.ta1.getText(), u'some ùnìcöd')
            self.ta1.onKeyDown(textarea.KEYCODES_BACKSPACE[1])
            self.ta1.onKeyDown(textarea.KEYCODES_BACKSPACE[0])
            self.assertEqual(self.ta1.getText(), u'some ùnìc')
            self.ta1.onKeyDown(ord(u'Ä'))
            self.assertEqual(self.ta1.getText(), u'some ùnìcÄ')

        def testSpecialChars():
            clear(self.ta1)
            self.ta1.onKeyDown(ord(u'&'))
            self.ta1.onKeyDown(textarea.KEYCODES_BACKSPACE[0])
            self.assertEqual(self.ta1.getText(), '')

        def checkSingleLine():
            text = ''
            self.ta2.setText('')
            while True:
                self.assert_(len(text) < 20)
                self.ta2.onKeyDown(ord(u'A'))
                text = text + 'A'
                if text != self.ta2.getText():
                    self.assertEqual(len(text), 16)
                    break

        root = self.loadEmptyScene()

        player.setFakeFPS(20)
        textarea.init(avg, False)
        self.start(True,
                (setup,
                 lambda: self.delay(200),
                 lambda: self.assertEqual(self.ta1.getText(), 'Lorem ipsum'),
                 lambda: setAndCheck(self.ta1, ''),
                 lambda: setAndCheck(self.ta2, 'Lorem Ipsum'),
                 testUnicode,
                 lambda: self.compareImage("testTextArea1"),
                 testSpecialChars,
                 checkSingleLine,
                 lambda: self.compareImage("testTextArea2"),
                 lambda: self.ta2.showCursor(True),
                 lambda: self.delay(200),
                 lambda: self._sendTouchEvent(1, avg.CURSORDOWN, 30, 100),
                 lambda: self._sendTouchEvent(1, avg.CURSORUP, 30, 100),
                 lambda: self.compareImage("testTextArea3"),
                 lambda: self._sendTouchEvent(2, avg.CURSORDOWN, 130, 100),
                 lambda: self.delay(1100),
                 lambda: self.compareImage("testTextArea4"),
                 lambda: self._sendTouchEvent(2, avg.CURSORMOTION, 30, 100),
                 lambda: self.compareImage("testTextArea5"),
                 lambda: self._sendTouchEvent(2, avg.CURSORUP, 30, 100),
                 lambda: self.compareImage("testTextArea3"),
                ))
        player.setFakeFPS(-1)

    def testFocusContext(self):
        def setup():
            textarea.init(avg)
            self.ctx1 = textarea.FocusContext()
            self.ctx2 = textarea.FocusContext()

            self.ta1 = textarea.TextArea(self.ctx1, pos=(2,2), size=(156,54), parent=root)
            self.ta1.setStyle(font='Bitstream Vera Sans', variant='Roman',
                fontsize=16, multiline=True, color='FFFFFF')
            self.ta1.setText('Lorem ipsum')

            self.ta2 = textarea.TextArea(self.ctx1, pos=(2,58), size=(76,54), parent=root)
            self.ta2.setStyle(font='Bitstream Vera Sans', variant='Roman',
                fontsize=14, multiline=False, color='FFFFFF')
            self.ta2.setText('dolor')

            self.bgImage = avg.ImageNode(href="1x1_white.png", size=(76,54))
            self.ta3 = textarea.TextArea(self.ctx2, disableMouseFocus=True, pos=(80,58),
                size=(76,54), textBackgroundNode=self.bgImage, parent=root)
            self.ta3.setStyle(font='Bitstream Vera Sans', variant='Roman',
                fontsize=14, multiline=True, color='FFFFFF')
            self.ta3.setText('dolor sit amet')

            textarea.setActiveFocusContext(self.ctx1)

        def writeChar():
            helper = player.getTestHelper()
            helper.fakeKeyEvent(avg.KEYDOWN, 65, 65, "A", 65, 0)
            helper.fakeKeyEvent(avg.KEYUP, 65, 65, "A", 65, 0)
            helper.fakeKeyEvent(avg.KEYDOWN, 66, 66, "B", 66, 0)
            helper.fakeKeyEvent(avg.KEYUP, 66, 66, "B", 66, 0)
            helper.fakeKeyEvent(avg.KEYDOWN, 67, 67, "C", 67, 0)
            helper.fakeKeyEvent(avg.KEYUP, 67, 67, "C", 67, 0)

        def switchFocus():
            self.ctx1.cycleFocus()

        def clearFocused():
            self.ctx1.clear()

        def clickForFocus():
            self._sendMouseEvent(avg.CURSORDOWN, 20, 70)
            self._sendMouseEvent(avg.CURSORUP, 20, 70)

        root = self.loadEmptyScene()
        self.start(True,
                (setup,
                 lambda: self.compareImage("testFocusContext1"),
                 writeChar,
                 lambda: self.compareImage("testFocusContext2"),
                 switchFocus,
                 writeChar,
                 lambda: self.compareImage("testFocusContext3"),
                 switchFocus,
                 clearFocused,
                 lambda: self.compareImage("testFocusContext4"),
                 clickForFocus,
                 clearFocused,
                 lambda: self.compareImage("testFocusContext5"),
               ))


    def testButton(self):

        def onClick(event):
            self.clicked = True

        def reset():
            self.clicked = False

        def enable(enabled):
            button.enabled = enabled

        def createScene(**kwargs):
            root = self.loadEmptyScene()
            return ui.Button(
                    parent = root,
                    upNode = avg.ImageNode(href="button_up.png"),
                    downNode = avg.ImageNode(href="button_down.png"),
                    disabledNode = avg.ImageNode(href="button_disabled.png"),
                    clickHandler = onClick,
                    **kwargs
                    )

        def runTest():
            self.clicked = False
            self.start(False,
                    (# Standard down->up
                     lambda: self._sendTouchEvent(1, avg.CURSORDOWN, 0, 0),
                     lambda: self.assert_(not(self.clicked)),
                     lambda: self.compareImage("testUIButtonDown"),
                     lambda: self._sendTouchEvent(1, avg.CURSORUP, 0, 0),
                     lambda: self.assert_(self.clicked),
                     lambda: self.compareImage("testUIButtonUp"),

                     # Disable, down, up -> no click
                     reset,
                     lambda: self.assert_(button.enabled),
                     lambda: enable(False),
                     lambda: self.assert_(not(button.enabled)),
                     lambda: self.compareImage("testUIButtonDisabled"),
                     lambda: self._sendTouchEvent(2, avg.CURSORDOWN, 0, 0),
                     lambda: self._sendTouchEvent(2, avg.CURSORUP, 0, 0),
                     lambda: self.assert_(not(self.clicked)),
                     lambda: enable(True),
                     lambda: self.assert_(button.enabled),

                     # Down, up further away -> no click
                     reset,
                     lambda: self._sendTouchEvent(3, avg.CURSORDOWN, 0, 0),
                     lambda: self._sendTouchEvent(3, avg.CURSORUP, 100, 0),
                     lambda: self.assert_(not(self.clicked)),
                     lambda: self.compareImage("testUIButtonUp"),

                     # Down, move further away, up -> no click
                     reset,
                     lambda: self._sendTouchEvent(3, avg.CURSORDOWN, 0, 0),
                     lambda: self._sendTouchEvent(3, avg.CURSORMOTION, 100, 0),
                     lambda: self._sendTouchEvent(3, avg.CURSORUP, 100, 0),
                     lambda: self.assert_(not(self.clicked)),
                     lambda: self.compareImage("testUIButtonUp"),

                     # Test if button still reacts after abort
                     lambda: self._sendTouchEvent(4, avg.CURSORDOWN, 0, 0),
                     lambda: self.assert_(not(self.clicked)),
                     lambda: self.compareImage("testUIButtonDown"),
                     lambda: self._sendTouchEvent(4, avg.CURSORUP, 0, 0),
                     lambda: self.assert_(self.clicked),
                     lambda: self.compareImage("testUIButtonUp"),
                    ))

        button = createScene()
        runTest()

        button = createScene(activeAreaNode=avg.CircleNode(r=5, opacity=0))
        runTest()

        button = createScene(fatFingerEnlarge=True)
        runTest()

        root = self.loadEmptyScene()
        button = ui.BmpButton(
                parent = root,
                upSrc = "button_up.png",
                downSrc = "button_down.png",
                disabledSrc = "button_disabled.png",
                clickHandler = onClick
                )
        runTest()
       
        button = createScene(enabled=False)
        self.start(False,
                (lambda: self.compareImage("testUIButtonDisabled"),
                ))

    def testToggleButton(self):

        def onCheck(event, isChecked):
            self.checked = isChecked
            self.checkedChanged = True
        
        def reset():
            self.checked = False
            self.checkedChanged = False

        def createScene(**kwargs):
            root = self.loadEmptyScene()
            return ui.ToggleButton(
                    uncheckedUpNode = avg.ImageNode(href="toggle_unchecked_Up.png"),
                    uncheckedDownNode = avg.ImageNode(href="toggle_unchecked_Down.png"),
                    checkedUpNode = avg.ImageNode(href="toggle_checked_Up.png"),
                    checkedDownNode = avg.ImageNode(href="toggle_checked_Down.png"),
                    uncheckedDisabledNode =
                            avg.ImageNode(href="toggle_unchecked_Disabled.png"),
                    checkedDisabledNode =
                            avg.ImageNode(href="toggle_checked_Disabled.png"),
                    checkHandler=onCheck,
                    parent=root,
                    **kwargs
                   )

        def testToggle():
            self.start(False,
                    (reset,
                     lambda: self.compareImage("testUIToggleUnchecked_Up"),
                     lambda: self._sendTouchEvent(1, avg.CURSORDOWN, 0, 0),
                     lambda: self.assert_(not(self.checked) and not(self.checkedChanged)),
                     lambda: self.compareImage("testUIToggleUnchecked_Down"),
                     lambda: self._sendTouchEvent(1, avg.CURSORUP, 0, 0),
                     lambda: self.assert_(self.checked and self.checkedChanged),
                     lambda: self.compareImage("testUIToggleChecked_Up"),
                     lambda: self._sendTouchEvent(2, avg.CURSORDOWN, 0, 0),
                     lambda: self.compareImage("testUIToggleChecked_Down"),
                     lambda: self._sendTouchEvent(2, avg.CURSORUP, 0, 0),
                     lambda: self.assert_(not(self.checked) and self.checkedChanged),
                     lambda: self.compareImage("testUIToggleUnchecked_Up"),
                    ))

        def testToggleAbort():
            self.start(False,
                    (reset,
                     lambda: self.compareImage("testUIToggleUnchecked_Up"),
                     lambda: self._sendTouchEvent(1, avg.CURSORDOWN, 0, 0),
                     lambda: self.compareImage("testUIToggleUnchecked_Down"),
                     lambda: self._sendTouchEvent(1, avg.CURSORUP, 100, 0),
                     lambda: self.assert_(not(self.checkedChanged)),
                     lambda: self.compareImage("testUIToggleUnchecked_Up"),
                     lambda: button.setChecked(True),
                     lambda: self.compareImage("testUIToggleChecked_Up"),
                     lambda: self._sendTouchEvent(2, avg.CURSORDOWN, 0, 0),
                     lambda: self.compareImage("testUIToggleChecked_Down"),
                     lambda: self._sendTouchEvent(2, avg.CURSORUP, 100, 0),
                     lambda: self.assert_(not(self.checkedChanged)),
                     lambda: self.compareImage("testUIToggleChecked_Up"),
                    ))

        def testToggleDisable():
            self.start(False,
                    (reset,
                     lambda: self.compareImage("testUIToggleUnchecked_Disabled"),
                     lambda: self._sendTouchEvent(1, avg.CURSORDOWN, 0, 0),
                     lambda: self._sendTouchEvent(1, avg.CURSORUP, 0, 0),
                     lambda: self.compareImage("testUIToggleUnchecked_Disabled"),
                     lambda: button.setEnabled(True),
                     lambda: self.compareImage("testUIToggleUnchecked_Up"),
                     lambda: self._sendTouchEvent(2, avg.CURSORDOWN, 0, 0),
                     lambda: button.setEnabled(False),
                     lambda: self._sendTouchEvent(2, avg.CURSORUP, 0, 0),
                     lambda: self.assert_(not(self.checked)),
                     lambda: self.compareImage("testUIToggleUnchecked_Disabled"),
                     
                     lambda: button.setEnabled(True),
                     reset,
                     lambda: self.compareImage("testUIToggleUnchecked_Up"),
                     lambda: button.setChecked(True),
                     lambda: self.compareImage("testUIToggleChecked_Up"),
                     lambda: button.setEnabled(False),
                     lambda: self.compareImage("testUIToggleChecked_Disabled"),
                     lambda: self._sendTouchEvent(3, avg.CURSORDOWN, 0, 0),
                     lambda: self._sendTouchEvent(3, avg.CURSORUP, 0, 0),
                     lambda: self.compareImage("testUIToggleChecked_Disabled"),
                     lambda: button.setEnabled(True),
                     lambda: self.compareImage("testUIToggleChecked_Up"),
                     lambda: self._sendTouchEvent(4, avg.CURSORDOWN, 0, 0),
                     lambda: button.setEnabled(False),
                     lambda: self._sendTouchEvent(4, avg.CURSORUP, 0, 0),
                     lambda: self.assert_(not(self.checkedChanged)),
                     lambda: self.compareImage("testUIToggleChecked_Disabled"),
                    ))
       
        def testFromSrc():
            root = self.loadEmptyScene()
            button = ui.BmpToggleButton(
                    uncheckedUpSrc="toggle_unchecked_Up.png",
                    uncheckedDownSrc="toggle_unchecked_Down.png",
                    checkedUpSrc="toggle_checked_Up.png",
                    checkedDownSrc="toggle_checked_Down.png",
                    uncheckedDisabledSrc="toggle_unchecked_Disabled.png",
                    checkedDisabledSrc="toggle_checked_Disabled.png",
                    checkHandler=onCheck,
                    parent=root)
            self.start(False,
                    (lambda: self.compareImage("testUIToggleUnchecked_Up"),
                     lambda: button.setChecked(True),
                     lambda: self.compareImage("testUIToggleChecked_Up"),
                     lambda: button.setChecked(False),
                     lambda: button.setEnabled(False),
                     lambda: self.compareImage("testUIToggleUnchecked_Disabled"),
                     lambda: button.setChecked(True),
                     lambda: self.compareImage("testUIToggleChecked_Disabled"),
                    ))
 
        button = createScene()
        testToggle()
        
        button = createScene()
        testToggleAbort()
        
        button = createScene(enabled=False)
        testToggleDisable()

        button = createScene(activeAreaNode = avg.CircleNode(r=5, opacity=0))
        testToggle()
        
        button = createScene(fatFingerEnlarge = True)
        testToggle()

        testFromSrc()

    def testScrollPane(self):
        def scrollLarge():
            scrollPane.contentpos = (-34, -34)
            self.assertEqual(scrollPane.contentpos, (-32,-32))

        def initSmallContent():
            scrollPane.size = (64, 64)
            contentArea.size = (32, 32)
            image.size = (32, 32)
            scrollPane.contentpos = (0, 0)
            self.assertEqual(scrollPane.getMaxContentPos(), (32,32))

        def scrollSmall():
            scrollPane.contentpos = (32, 32)

        root = self.loadEmptyScene()
        contentArea = avg.DivNode(size=(64,64))
        image = avg.ImageNode(href="rgb24-64x64.png", parent=contentArea)
        scrollPane = ui.ScrollPane(contentNode=contentArea, size=(32,32), parent=root)

        self.start(False,
                (lambda: self.compareImage("testScrollPane1"),
                 scrollLarge,
                 lambda: self.compareImage("testScrollPane2"),
                 initSmallContent,
                 lambda: self.compareImage("testScrollPane3"),
                 scrollSmall,
                 lambda: self.compareImage("testScrollPane4"),
                ))

    def testAccordionNode(self):
        def createNode():
            self.node = ui.AccordionNode(src="media/rgb24-32x32.png", endsExtent=15, 
                    orientation=orientation, parent=root)
            
        def changeExtent():
            self.node.extent = 100

        def minExtent():
            self.node.extent = 3
            self.assert_(self.node.extent == 31)

        for orientation, orName in (
                (ui.Orientation.HORIZONTAL,"Horiz"),
                (ui.Orientation.VERTICAL, "Vert")):
            root = self.loadEmptyScene()
            self.start(False,
                    (createNode,
                     lambda: self.compareImage("testAccordionNode"+orName+"1"),
                     changeExtent,
                     lambda: self.compareImage("testAccordionNode"+orName+"2"),
                     minExtent,
                     lambda: self.compareImage("testAccordionNode"+orName+"1"),
                    ))

    def testScrollBar(self):
        def createNode(orientation):
            if orientation == ui.Orientation.HORIZONTAL:
                self.node = ui.BmpScrollBar(orientation=orientation,
                        trackSrc="media/scrollbar_horiz_track.png",
                        trackDisabledSrc="media/scrollbar_horiz_track_disabled.png",
                        trackEndsExtent=2,
                        thumbUpSrc="media/scrollbar_horiz_thumb_up.png",
                        thumbDownSrc="media/scrollbar_horiz_thumb_down.png",
                        thumbDisabledSrc="media/scrollbar_horiz_thumb_disabled.png",
                        thumbEndsExtent=4,
                        width=100,
                        parent=root)
            else:
                self.node = ui.BmpScrollBar(orientation=orientation,
                        trackSrc="media/scrollbar_vert_track.png",
                        trackDisabledSrc="media/scrollbar_vert_track_disabled.png",
                        trackEndsExtent=2,
                        thumbUpSrc="media/scrollbar_vert_thumb_up.png",
                        thumbDownSrc="media/scrollbar_vert_thumb_down.png",
                        thumbDisabledSrc="media/scrollbar_vert_thumb_disabled.png",
                        thumbEndsExtent=4,
                        height=100,
                        parent=root)

        def onThumbPosChanged(pos):
            self.thumbpos = pos

        for orientation, orName in (
                (ui.Orientation.HORIZONTAL,"Horiz"),
                (ui.Orientation.VERTICAL, "Vert")):
            root = self.loadEmptyScene()
            self.start(False,
                    (lambda: createNode(orientation),
                     lambda: self.compareImage("testScrollBar"+orName+"1"),
                     lambda: self.node.setThumbExtent(0.5),
                     lambda: self.compareImage("testScrollBar"+orName+"2"),
                     lambda: self.node.setThumbPos(0.5),
                     lambda: self.compareImage("testScrollBar"+orName+"3"),
                     lambda: self.node.setThumbPos(1),
                     lambda: self.compareImage("testScrollBar"+orName+"4"),
                     lambda: self.node.setRange((0,10)),
                     lambda: self.node.setThumbPos(5),
                     lambda: self.compareImage("testScrollBar"+orName+"5")
                    ))

        # Horizontal
        root = self.loadEmptyScene()
        self.start(False,
                (lambda: createNode(ui.Orientation.HORIZONTAL),
                 lambda: self.node.setThumbExtent(0.5),
                 # User input
                 lambda: self._sendMouseEvent(avg.CURSORDOWN, 25, 10),
                 lambda: self.compareImage("testScrollBarHoriz6"),
                 lambda: self._sendMouseEvent(avg.CURSORMOTION, 50, 10),
                 lambda: self.compareImage("testScrollBarHoriz7"),
                 lambda: self.assertAlmostEqual(self.node.getThumbPos(), 0.5),
                 lambda: self._sendMouseEvent(avg.CURSORMOTION, 25, 10),
                 lambda: self.compareImage("testScrollBarHoriz8"),
                 lambda: self.assertAlmostEqual(self.node.getThumbPos(), 0),
                 lambda: self._sendMouseEvent(avg.CURSORUP, 0, 10),
                 lambda: self.compareImage("testScrollBarHoriz9"),
                 lambda: self.assertAlmostEqual(self.node.getThumbPos(), 0),

                 # Publish/Subscribe interface
                 lambda: self.node.subscribe(ui.ScrollBar.THUMB_POS_CHANGED, 
                        onThumbPosChanged),
                 lambda: self._sendMouseEvent(avg.CURSORDOWN, 25, 10),
                 lambda: self._sendMouseEvent(avg.CURSORUP, 50, 10),
                 lambda: self.assertAlmostEqual(self.thumbpos, 0.5),

                 # Enable/disable
                 lambda: self.node.setEnabled(False),
                 lambda: self.compareImage("testScrollBarHoriz10"),
                 lambda: self._sendMouseEvent(avg.CURSORDOWN, 50, 10),
                 lambda: self._sendMouseEvent(avg.CURSORUP, 25, 10),
                 lambda: self.assertAlmostEqual(self.thumbpos, 0.5),
                 lambda: self.node.setEnabled(True),
                 lambda: self.compareImage("testScrollBarHoriz11"),

                 # Disable after down: Drag aborted
                 lambda: self._sendMouseEvent(avg.CURSORDOWN, 50, 10),
                 lambda: self.node.setEnabled(False),
                 lambda: self._sendMouseEvent(avg.CURSORUP, 25, 10),
                 lambda: self.assertAlmostEqual(self.thumbpos, 0.5),
                 lambda: self.node.setEnabled(True),
                 lambda: self.compareImage("testScrollBarHoriz11"),
                ))

        # Vertical
        root = self.loadEmptyScene()
        self.start(False,
                (lambda: createNode(ui.Orientation.VERTICAL),
                 lambda: self.node.setThumbExtent(0.5),
                 # User input
                 lambda: self._sendMouseEvent(avg.CURSORDOWN, 10, 25),
                 lambda: self.compareImage("testScrollBarVert6"),
                 lambda: self._sendMouseEvent(avg.CURSORMOTION, 10, 50),
                 lambda: self.compareImage("testScrollBarVert7"),
                 lambda: self.assertAlmostEqual(self.node.getThumbPos(), 0.5),
                 lambda: self._sendMouseEvent(avg.CURSORMOTION, 10, 25),
                 lambda: self.compareImage("testScrollBarVert8"),
                 lambda: self.assertAlmostEqual(self.node.getThumbPos(), 0),
                 lambda: self._sendMouseEvent(avg.CURSORUP, 10, 0),
                 lambda: self.compareImage("testScrollBarVert9"),
                 lambda: self.assertAlmostEqual(self.node.getThumbPos(), 0),
                ))

    def testSimpleScrollBar(self):

        def createNode():
            if orientation == ui.Orientation.HORIZONTAL:
                size = (100, 20)
            else:
                size = (20, 100)
            self.node = ui.simple.ScrollBar(size=size, orientation=orientation,
                    parent=root)

        for orientation, orName in (
                (ui.Orientation.HORIZONTAL,"Horiz"),
                (ui.Orientation.VERTICAL, "Vert")):
            root = self.loadEmptyScene()
            self.start(False,
                    (lambda: createNode(),
                     lambda: self.compareImage("testSimpleScrollBar"+orName+"1"),
                     lambda: self.node.setThumbPos(1),
                     lambda: self.compareImage("testSimpleScrollBar"+orName+"2"),
                     lambda: self.node.setExtent(50),
                     lambda: self.compareImage("testSimpleScrollBar"+orName+"3"),
                    ))

    def testSimpleSlider(self):

        def createNode():
            if orientation == ui.Orientation.HORIZONTAL:
                size = (100, 20)
            else:
                size = (20, 100)
            self.node = ui.simple.Slider(size=size, orientation=orientation,
                    parent=root)

        for orientation, orName in (
                (ui.Orientation.HORIZONTAL,"Horiz"),
                (ui.Orientation.VERTICAL, "Vert")):
            root = self.loadEmptyScene()
            self.start(False,
                    (lambda: createNode(),
                     lambda: self.compareImage("testSimpleSlider"+orName+"1"),
                     lambda: self.node.setThumbPos(1),
                     lambda: self.compareImage("testSimpleSlider"+orName+"2"),
                     lambda: self.node.setExtent(50),
                     lambda: self.compareImage("testSimpleSlider"+orName+"3"),
                    ))


def uiTestSuite(tests):
    availableTests = (
        "testKeyboard",
        "testTextArea",
        "testFocusContext",
        "testButton",
        "testToggleButton",
        "testScrollPane",
        "testAccordionNode",
        "testScrollBar",
        "testSimpleScrollBar",
        "testSimpleSlider"
        )

    return createAVGTestSuite(availableTests, UITestCase, tests)
