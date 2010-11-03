#!/usr/bin/env python
# -*- coding: utf-8 -*-

from libavg import avg

player = avg.Player.get()

player.loadString("""<avg width="160" height="120" id="container" />""")
# Change following line if the plugin is somewhere else.
player.pluginPath = "../test/plugin/.libs"
player.loadPlugin("libColorNode")

node = player.createNode('<colornode fillcolor="0f3f7f" id="mynode" />')
player.getElementByID("container").appendChild(node)
mynode = player.getElementByID("mynode")
mynode.fillcolor = "7f007f"
player.play()
