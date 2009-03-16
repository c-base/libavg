#!/usr/bin/python
# -*- coding: utf-8 -*-


import sys, os, math, urllib, re, datetime, time, random, thread, gc
sys.path.append('/usr/local/lib/python2.4/site-packages/libavg')
import avg

sys.path.append('../')
import anim

def step_anim():
    pass

def start_anim():
#    id = Player.setInterval(1, step_anim)
#    Player.setTimeout(5, lambda: Player.clearInterval(id))
    curTermin = Player.getElementByID("linie1")
    anim.SplineAnim(curTermin, "x", 100, -800, 2000, 10, -20, None)
    print gc.collect()

class autonomous:
    def __init__(self):
        self.__id = Player.setInterval(1, step_anim)
#        self.__id = Player.setInterval(1, self.__step)
        Player.setTimeout(10, lambda: Player.clearInterval(self.__id))
        print gc.collect()
    def __del__(self):
        print("del")
    def __step(self):
        pass

class Termin:
    def __init__(self, date, time, event):
        def escape(s):
            return s.replace("&", "&amp;")
        self.date = escape(date)
        self.time = escape(time)
        self.event = escape(event)
        print (self.date, self.time, self.event)

def parse_termine():
    global termineStr
    global termine
    global termineBereit
    termine = []
    lines = termineStr.splitlines()
    expr = re.compile(
            "\|\|'''(.+)'''\|\|(.+)\|\|'''(.+)'''\|\|'''(.+)'''\|\|'''(.+)'''\|\|")
    for line in lines:
        line = line.rstrip("\n\r \t")
        match = expr.search(line)
        if match != None:
            if match.group(5) == "x" or match.group(5) == "xx":
                date_struct = time.strptime(match.group(1), "%d.%m.%Y")
                eventDate = datetime.date(date_struct.tm_year, date_struct.tm_mon, 
                        date_struct.tm_mday)
                if match.group(5) == "x":
                    td = datetime.timedelta(30)
                else:
                    td = datetime.timedelta(60)
                today = datetime.date.today()
                if (eventDate >= today and eventDate < today+td):
                    termine.append(Termin(
                            match.group(1), match.group(3), match.group(4)));
    termineBereit = 0
    
    

def load_termine():
    global termineStr
    print "Termine werden gelesen."
    file = urllib.urlopen("http://coredump.c-base.info/TerMine?action=raw")
    print "Termine fertig gelesen."
    termineStr = file.read()
    parse_termine()

exiting = 0
termineBereit = 0

def termin_watcher():
    global exiting
    global termineBereit
    global termineStr
    while not(exiting):
        time.sleep(60)
        print "Termine werden gelesen."
        file = urllib.urlopen("http://coredump.c-base.info/TerMine?action=raw")
        print "Termine fertig gelesen."
        termineStr = file.read()
        termineBereit = 1

 
letzteIndices = [0, 0, 0]

def start_termin():
    global curTerminNum
    global terminVonLinks
    global termine
    global letzteIndices
    curInfoIndex = int(random.random()*len(termine))
    letzteIndices.append(curInfoIndex)
    letzteIndices = letzteIndices[1:]
    curInfo = termine[curInfoIndex]
    curTermin = Player.getElementByID("linie"+str(curTerminNum))
    topLine = Player.getElementByID("linie"+str(curTerminNum)+"_top")
    topLine.text = curInfo.event
    bottomLine = Player.getElementByID("linie"+str(curTerminNum)+"_bottom")
    bottomLine.text = curInfo.date+", "+curInfo.time
    if terminVonLinks:
        anim.SplineAnim(curTermin, "x", 100, -800, 2000, 10, -20, None)
    else:
        anim.SplineAnim(curTermin, "x", 100, 800, -2000, -10, 20, None)
    Player.setTimeout(10, termin_weg)
    if termineBereit:
        parse_termine()

def termin_weg():
    global curTerminNum
    global terminVonLinks
    curTerminNum += 1
    if curTerminNum == 4:
        curTerminNum = 1
    terminVonLinks = (random.random() > 0.5)
    curTermin = Player.getElementByID("linie"+str(curTerminNum))
    if terminVonLinks:
        anim.SplineAnim(curTermin, "x", 100, 0, 0, 800, -2000, None)
    else:
        anim.SplineAnim(curTermin, "x", 140, 0, 0, -1200, 2000, None)
    Player.setTimeout(10, start_termin)
    print gc.collect()


def init_termine():
    global curTerminNum
    global terminVonLinks
    load_termine()
    Player.getElementByID("linie1").x=900
    Player.getElementByID("linie2").x=900
    Player.getElementByID("linie3").x=900
    curTerminNum = 1
    terminVonLinks = 0 
    Player.setTimeout(10, start_termin)

Player = avg.Player.get()
Log = avg.Logger.get()
Player.setResolution(0, 0, 0, 0) 
Log.setCategories(Log.APP |
                  Log.WARNING | 
                  Log.PROFILE |
                  Log.PROFILE_LATEFRAMES |
                  Log.CONFIG 
#                 Log.MEMORY  |
#                 Log.BLTS    |
#                  Log.EVENTS
                  )
gc.set_debug(gc.DEBUG_COLLECTABLE | gc.DEBUG_UNCOLLECTABLE | gc.DEBUG_INSTANCES | 
        gc.DEBUG_OBJECTS | gc.DEBUG_STATS)
Player.loadFile("testleak.avg")
anim.init(Player)
#init_termine()
Player.setInterval(5, start_anim)
#Player.setInterval(5, autonomous)
Player.setFramerate(1000)
Player.play()
done = 1
