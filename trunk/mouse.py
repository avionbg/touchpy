#!/usr/bin/python -u

from touch import touchlib

class Observer(object):
	def __init__(self, subject):
		subject.push_handlers(self)

class touch_up(Observer):
	def on_touchup(self,blobID, xpos, ypos):
		print 'blob release detected: ', blobID, xpos, ypos
		pass

class touch_down(Observer):
	def on_touchdown(self,blobID):
		print 'blob press detected: ', blobID.sessionid, blobID.xpos, blobID.ypos
		pass
class touch_move(Observer):
	def on_touchmove(self,blobID):
		#tuio_cmd.comm.xo=int(blobID.oxpos * 1650)
		#tuio_cmd.comm.yo=int(blobID.oypos * 1050)
		#print "blobs move:", int(blobID.xpos * 1650), int(blobID.ypos *1050)
		#print "old:",int(blobID.oxpos * 1650)
		pass

t = touchlib()
tu = touch_up(t)
td = touch_down(t)
tm = touch_move(t)
windows = {}
wid = {}



try:
	while True:
		t.dispatch_events()

except (KeyboardInterrupt, SystemExit):
	t.stop()
