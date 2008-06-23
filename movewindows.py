#!/usr/bin/python -u
"""
Example of moving the windows with touchpy
"""
from touch import *
import wmxlibutil

class Window(object):
	def __init__(self,sessionID,wid):
		self.ID = sessionID
		self.numblobs = 1
		self.wid = wid
		#attr = getWindowAttributes(wid)
		#self.x = attr['x']
		#self.y = attr['y']
		self.width = width
		self.height = height

	def addBlob(self,blobID):
		self.blobs.append = blobID
	def removeBlob(self,blobID):
		self.blobs.pop(blobID)

class Observer(object):
	def __init__(self, subject):
		subject.push_handlers(self)

class TOUCH_UP(Observer):
	def TOUCH_UP(self,blobID, xpos, ypos):
		if DEBUG: print 'blob release detected: ', blobID, xpos, ypos
		for window in wid:
			if DEBUG: print window,wid[window].ID,blobID
			if wid[window].ID == blobID:
				del wid[window]
				break
		pass

class TOUCH_DOWN(Observer):
	def TOUCH_DOWN(self,blobID):
		w_id = wmxlibutil.pointer2wid(int(t.blobs[blobID].xpos*width),int(t.blobs[blobID].ypos*height))
		wid[w_id] = Window(blobID,w_id)
		if DEBUG: print 'blob press detected: ', blobID, t.blobs[blobID].xpos, t.blobs[blobID].ypos
		pass

class TOUCH_MOVE(Observer):
	def TOUCH_MOVE(self,blobID):
		if DEBUG: print 'blob move detected: ', blobID, t.blobs[blobID].xpos, t.blobs[blobID].ypos
		for window in wid:
			if wid[window].ID == blobID:
				wmxlibutil.window_move (window,int(t.blobs[blobID].xpos * width), int(t.blobs[blobID].ypos *height))
		pass

(width,height) = wmxlibutil.getdisplaysize()

t = touchpy()
tu = TOUCH_UP(t)
td = TOUCH_DOWN(t)
tm = TOUCH_MOVE(t)
windows = {}
wid = {}
DEBUG = 1
try:
	while True:
		t.update()

except (KeyboardInterrupt, SystemExit):
	del t
