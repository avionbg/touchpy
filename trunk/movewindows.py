#!/usr/bin/python -u
"""
Example of moving the windows with touchpy
"""
from touch import touchlib
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

class touch_up(Observer):
	def on_touchup(self,blobID, xpos, ypos):
		if DEBUG: print 'blob release detected: ', blobID, xpos, ypos
		for window in wid:
			if DEBUG: print window,wid[window].ID,blobID
			if wid[window].ID == blobID:
				del wid[window]
				break
		pass

class touch_down(Observer):
	def on_touchdown(self,blobID):
		w_id = wmxlibutil.pointer2wid(int(blobID.xpos*width),int(blobID.ypos*height))
		wid[w_id] = Window(blobID.sessionid,w_id)
		if DEBUG: print 'blob press detected: ', blobID.sessionid, blobID.xpos, blobID.ypos
		pass

class touch_move(Observer):
	def on_touchmove(self,blobID):
		if DEBUG: print 'blob move detected: ', blobID.sessionid, blobID.xpos, blobID.ypos
		for window in wid:
			if wid[window].ID == blobID.sessionid:
				wmxlibutil.window_move (window,int(blobID.xpos * width), int(blobID.ypos *height))
		pass

(width,height) = wmxlibutil.getdisplaysize()

t = touchlib()
tu = touch_up(t)
td = touch_down(t)
tm = touch_move(t)
windows = {}
wid = {}
DEBUG = 1
try:
	while True:
		t.dispatch_events()

except (KeyboardInterrupt, SystemExit):
	t.stop()
