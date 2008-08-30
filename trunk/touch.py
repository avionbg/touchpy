#!/usr/bin/python -u
# coding: utf-8

#import os, time
import event
#import liblo
import sets
import sys
import socket

def is_module_available (module) :
    try :
        exec('import %s' % module)
        exec('del %s' % module)
    except ImportError :
        return False
    else :
        return True

def intersection(set1,set2): return filter(lambda s:s in set2,set1)

def difference(set1,set2): return filter(lambda s:s not in set2,set1)

class Tuio2DCursor(event.EventDispatcher):
	def __init__(self, blobID,args):
		self.blobID = blobID
		self.oxpos = self.oypos = 0.0
		if len(args) == 5:
			self.xpos, self.ypos, self.xmot, self.ymot, self.mot_accel = args[0:5]
		else:
			self.xpos, self.ypos, self.xmot, self.ymot, self.mot_accel, self.Width , self.Height = args[0:7]

	def move(self, args):
		self.oxpos, self.oypos = self.xpos, self.ypos
		if len(args) == 5:
			self.xpos, self.ypos, self.xmot, self.ymot, self.mot_accel = args[0:5]
		else:
			self.xpos, self.ypos, self.xmot, self.ymot, self.mot_accel, self.Width , self.Height = args[0:7]

class touchpy(event.EventDispatcher):
	def __init__(self, host='127.0.0.1', port=3333):
		self.current_frame = self.last_frame = 0
		self.alive = []
		self.blobs = {}
		self.address = "/tuio/2Dcur"
		if is_module_available("libloo"):
			self.liblo = True
			try:
				import liblo
				self.server = liblo.Server(port)
			except liblo.ServerError, err:
				sys.exit(str(err))
			self.server.add_method(self.address, None, self.handle2Dcur)
		else:
			self.liblo = False
			import OSC
			self.socket = socket.socket(socket.AF_INET, socket.SOCK_DGRAM)
			self.socket.setsockopt(socket.SOL_SOCKET, socket.SO_REUSEADDR, 1)
			self.socket.setblocking(0)
			self.socket.bind((host,port))
			self.rawosc = OSC.CallbackManager()
			self.rawosc.add(self.fallback, self.address)

	def close_socket(self):
		self.socket.close()

	def fallback(self, *incoming):
		message = incoming[0]
		path, types, args = message[0], message[1], message[2:]
		if (path == self.address):
			self.handle2Dcur(path,args,types,"raw")

	def handle2Dcur(self, path, args, types, src):
		if args[0] == 'alive':
			touch_release = difference(self.alive,args[1:])
			touch_down = difference(self.alive,args[1:])
			touch_move = intersection(self.alive,args[1:])
			self.alive = args[1:]
			for blobID in touch_release:
				self.dispatch_event('TOUCH_UP', blobID, self.blobs[blobID].xpos, self.blobs[blobID].ypos)
				del self.blobs[blobID]

		elif args[0] == 'set':
			blobID = args[1]
			if blobID not in self.blobs:
				self.blobs[blobID] = Tuio2DCursor(blobID,args[2:])
				#self.blobs[blobID].update(args[2:])
				self.dispatch_event('TOUCH_DOWN', blobID)
			else:
				self.blobs[blobID].move(args[2:])
				self.dispatch_event('TOUCH_MOVE', blobID)

		elif args[0] == 'fseq':
			self.last_frame = self.current_frame
			self.current_frame = args[1]
			self.dispatch_event('FSEQ', self.current_frame)
			#print 'fseq',self.current_frame

	def TOUCH_DOWN(self, blobID):
		pass

	def TOUCH_UP(self, blobID, x, y):
		pass

	def TOUCH_MOVE(self, blobID):
		pass

	def FSEQ(self, framenum):
		pass

	def update(self):
		#while True:
		if self.liblo:
			self.server.recv(0)
		else:
			try:
				self.rawosc.handle(self.socket.recv(1024))
			except socket.error:
				pass

touchpy.register_event_type('TOUCH_DOWN')
touchpy.register_event_type('TOUCH_UP')
touchpy.register_event_type('TOUCH_MOVE')
touchpy.register_event_type('FSEQ')

if __name__ == '__main__':
	t = touchpy()
	@t.event
	def TOUCH_DOWN(blobID):
		print 'blob press detected: ', blobID, t.blobs[blobID].xpos, t.blobs[blobID].ypos

	@t.event
	def TOUCH_UP(blobID,x,y):
		print 'blob release detected: ', blobID, x, y

	@t.event
	def TOUCH_MOVE(blobID):
		print 'blob move detected: ', blobID, t.blobs[blobID].xpos, t.blobs[blobID].ypos
	try:
		while True:
			t.update()
			#for obj in t.blobs:
				#print t.blobs[obj].blobID, t.blobs[obj].xpos, t.blobs[obj].ypos

	except KeyboardInterrupt:
		del t
