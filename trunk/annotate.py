#!/usr/bin/python -u

from touch import touchlib
import shm
import ctypes

class tuio(ctypes.Structure):
	_fields_ = [
		('x',ctypes.c_int),
		('y',ctypes.c_int),
		('xo',ctypes.c_int),
		('yo',ctypes.c_int),
		('id',ctypes.c_int)
	]
class nested(ctypes.Structure):
	_fields_=[
		('lock',ctypes.c_int),
		('comm',tuio*20)
	]

key = 5679
mem = shm.memory(shm.getshmid(key))
mem.attach()

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
		sem.setval(0)
		tuio_cmd.comm[0].x=int(blobID.xpos * 1650)
		tuio_cmd.comm[0].y=int(blobID.ypos * 1050)
		tuio_cmd.comm[0].xo=int(blobID.oxpos * 1650)
		tuio_cmd.comm[0].yo=int(blobID.oypos * 1050)
		sem.Z()
		mem.write(tuio_cmd)
		sem.setval(2)
		#print "blobs move:", int(blobID.xpos * 1650), int(blobID.ypos *1050)
		#print "old:",int(blobID.oxpos * 1650)
		pass

t = touchlib()
tu = touch_up(t)
td = touch_down(t)
tm = touch_move(t)
windows = {}
wid = {}
key = 5679
mem = shm.memory(shm.getshmid(key))
sem = shm.semaphore(shm.getsemid(key))
mem.attach()

tuio_cmd = nested()


try:
	while True:
		t.dispatch_events()

except (KeyboardInterrupt, SystemExit):
	t.stop()
