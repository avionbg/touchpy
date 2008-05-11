#!/usr/bin/python -u
# coding: utf-8

import tuio
import os, time
import sets
from pyglet import event
import copy

def intersection(set1,set2): return filter(lambda s:s in set2,set1)

def difference(set1,set2): return filter(lambda s:s not in set2,set1)

class touchlib(event.EventDispatcher):
	def __init__(self, host='127.0.0.1', port=3333):
		global tracking,old_blobs,new_blobs,old_tobje
		self.host = host
		self.port = port
		old_blobs = []
		old_tobje = {}
		#try:
		tracking = tuio.Tracking(host=self.host, port=self.port)
		#except (KeyboardInterrupt, SystemExit):
			#tracking.stop()
	def stop(self):
		tracking.stop()

	def dispatch_events(self):
		global old_blobs, old_tobje
		tracking.update()
		new_blobs = tracking.profiles['/tuio/2Dcur'].sessions
		if new_blobs != old_blobs:
			touch_release = difference(old_blobs,new_blobs)
			touch_down = difference(new_blobs,old_blobs)
			touch_move = intersection(old_blobs,new_blobs)
			if touch_release:
				for blobID in touch_release:
					self.dispatch_event('on_touchup', blobID, old_tobje[blobID].xpos, old_tobje[blobID].ypos)
					del old_tobje[blobID]
			if touch_down:
				for blobID in touch_down:
					tobje = tracking.profiles['/tuio/2Dcur'].objects[blobID]
					self.dispatch_event('on_touchdown', tobje )
					old_tobje[blobID] = copy.copy(tobje)
			if touch_move:
				for blobID in touch_move:
					tobject = tracking.profiles['/tuio/2Dcur'].objects[blobID]
					self.dispatch_event('on_touchmove', tobject)
			old_blobs = new_blobs
		else:
			for blobID in new_blobs:
				tobje = tracking.profiles['/tuio/2Dcur'].objects[blobID]
				if ( (old_tobje[blobID].xpos != tobje.xpos) and (old_tobje[blobID].ypos != tobje.ypos) ):
					self.dispatch_event('on_touchmove', tobje)
					old_tobje[blobID] = copy.copy(tobje)
	def on_touchdown(self, tobject):
		pass

	def on_touchup(self, blobID, xpos, ypos):
		pass

	def on_touchmove(self, tobject):
		pass

touchlib.register_event_type('on_touchdown')
touchlib.register_event_type('on_touchup')
touchlib.register_event_type('on_touchmove')

class Observer(object):
    def __init__(self, subject):
        subject.push_handlers(self)

# Concrete observer
class tdown(Observer):
    def on_touchup(self,blobID, xpos, ypos):
	print 'blob release detected: ', blobID, xpos, ypos
	pass


if __name__ == "__main__":
	t = touchlib()
	import wutil
	DEBUG = 0
	td = tdown(t)
	#print wutil.pointer2wid(200,200)
	
	def movewin(x,y):
		#wutil.window_move(67108915,x,y)
		print wutil.getWindowUnderCursor()

	while True:
		t.dispatch_events()
	@t.event
	def on_touchdown(tobject):
		if DEBUG: print 'blob press detected: ', tobject.sessionid, tobject.xpos, tobject.ypos

	@t.event
	def on_touchup(blobID, xpos, ypos):
		if DEBUG: print 'blob release detected: ', blobID, xpos, ypos

	@t.event
	def on_touchmove(tobject):
		if DEBUG: print 'blob move detected: ', tobject.sessionid, tobject.xpos, tobject.ypos
		movewin (int(tobject.xpos * 1650), int(tobject.ypos *1050))

