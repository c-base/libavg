# Work around libstdc++ Mesa bug
# (https://bugs.launchpad.net/ubuntu/+source/mesa/+bug/259219)
from platform import system
if system() == 'Linux':
    from ctypes import cdll
    cdll.LoadLibrary("libpixman-1.so.0")
    cdll.LoadLibrary("libstdc++.so.6")
del system

from avg import *
player = avg.Player.get()

import textarea
import statemachine
from app import AVGApp, App
from appstarter import AVGAppStarter, AVGMTAppStarter, AppStarter
import utils
import gameapp
