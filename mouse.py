#!/usr/bin/python -u
"""
example mouse emulation with touchpy
"""
from touch import touchlib
import wmxlibutil

class Observer(object):
	def __init__(self, subject):
		subject.push_handlers(self)

class touch_up(Observer):
	def on_touchup(self,blobID, xpos, ypos):
		if DEBUG: print 'blob release detected: ', blobID, xpos, ypos
		wmxlibutil.mouse_click(1) #change this to have another button clicked
		pass

class touch_down(Observer):
	def on_touchdown(self,blobID):
		if DEBUG: print 'blob press detected: ', blobID.sessionid, blobID.xpos, blobID.ypos
		wmxlibutil.moveCursorTo(0,int(blobID.xpos*width),int(blobID.ypos*height))
		pass

class touch_move(Observer):
	def on_touchmove(self,blobID):
		if DEBUG: print 'blob move detected: ', blobID.sessionid, blobID.xpos, blobID.ypos
		wmxlibutil.moveCursorTo(0,int(blobID.xpos*width),int(blobID.ypos*height))
		pass

(width,height) = wmxlibutil.getdisplaysize()

t = touchlib()
tu = touch_up(t)
td = touch_down(t)
tm = touch_move(t)
DEBUG = 1

try:
	while True:
		t.dispatch_events()

except (KeyboardInterrupt, SystemExit):
	t.stop()
