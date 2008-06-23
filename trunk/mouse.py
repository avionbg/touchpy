#!/usr/bin/python -u
"""
example mouse emulation with touchpy
afcourse you can always make just one class to
handle all the events, this is just an example
"""
from touch import *
import wmxlibutil

class Observer(object):
	def __init__(self, subject):
		subject.push_handlers(self)

class touch_up(Observer):
	def TOUCH_UP(self,blobID, xpos, ypos):
		if DEBUG: print 'blob release detected: ', blobID, xpos, ypos
		wmxlibutil.mouse_click(1) #change this to have another button clicked
		pass

class touch_down(Observer):
	def TOUCH_DOWN(self,blobID):
		if DEBUG: print 'blob press detected: ', blobID, t.blobs[blobID].xpos, t.blobs[blobID].ypos
		wmxlibutil.moveCursorTo(0,int(t.blobs[blobID].xpos*width),int(t.blobs[blobID].ypos*height))
		pass

class touch_move(Observer):
	def TOUCH_MOVE(self,blobID):
		if DEBUG: print 'blob move detected: ', blobID, t.blobs[blobID].xpos, t.blobs[blobID].ypos
		wmxlibutil.moveCursorTo(0,int(t.blobs[blobID].xpos*width),int(t.blobs[blobID].ypos*height))
		pass

(width,height) = wmxlibutil.getdisplaysize()

t = touchpy()
tu = touch_up(t)
td = touch_down(t)
tm = touch_move(t)
DEBUG = 1

try:
	while True:
		t.update()

except (KeyboardInterrupt, SystemExit):
	del t
