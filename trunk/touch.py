#!/usr/bin/python -u
# coding: utf-8

import sets
from cursors import *

def intersection(set1,set2): return filter(lambda s:s in set2,set1)

def difference(set1,set2): return filter(lambda s:s not in set2,set1)

def test_import (module):
	"""Test if module exists and return true if so"""
	try :
		exec('import %s' % module)
		exec('del %s' % module)
	except ImportError :
		return False
	else :
		return True

'''Basic event for a basic event queue. This probably isn't the best
way to implement it, but its simple and it works'''
class Event:
	def __init__(self, type, blob):
		self.type = type
		self.blob = blob
		self.pos = [blob.xpos, blob.ypos]

class touchpy(event.EventDispatcher):
	def __init__(self, host='127.0.0.1', port=3333):
		self.current_frame = self.last_frame = 0
		self.cursorparser = Generic2DCursor
		self.alive = []
		self.blobs = {}
		self.events = []

		if test_import('liblo'):
			from llo import LibloParser
			self.parser = LibloParser(self.setup)

		else:
			from raw import RawParser
			self.parser = RawParser(self.setup)

	def setup(self, path, args, types, src):
		"""Setup cursor parser to Simulator(reactivision) format, or touchlib"""
		if args[0] == 'set':
			if len(args[2:]) == 5:
				self.cursorparser = Simul2DCursor
				self.provider = 'TUIOSimulator'
			else:
				self.cursorparser = Touch2DCursor
				self.provider = 'Touchlib'
		elif args[0] == 'fseq' and hasattr(self, 'provider'):
			self.parser.subst(self.handle2Dcur)
		self.handle2Dcur(path, args, types, src)

	def handle2Dcur(self, path, args, types, src):
		"""Do touch processing and fire events"""
		if args[0] == 'alive':
			touch_release = difference(self.alive,args[1:])
			#touch_down = difference(self.alive,args[1:])
			#touch_move = intersection(self.alive,args[1:])
			self.alive = args[1:]
			for blobID in touch_release:
				self.dispatch_event('TOUCH_UP', blobID, self.blobs[blobID].xpos, self.blobs[blobID].ypos)
				self.events.append(Event('TOUCH_UP', self.blobs[blobID]))
				del self.blobs[blobID]

		elif args[0] == 'set':
			blobID = args[1]
			if blobID not in self.blobs:
				self.blobs[blobID] = self.cursorparser(blobID,args[2:])
				self.dispatch_event('TOUCH_DOWN', blobID)
				self.events.append(Event('TOUCH_DOWN', self.blobs[blobID]))
			else:
				self.blobs[blobID].move(args[2:])
				self.dispatch_event('TOUCH_MOVE', blobID)
				self.events.append(Event('TOUCH_MOVE', self.blobs[blobID]))


		elif args[0] == 'fseq':
			self.last_frame = self.current_frame
			self.current_frame = args[1]
			self.dispatch_event('FSEQ', self.current_frame)

	def update(self):
		#Clear the event queue
		self.events = []
		"""Process next packets"""
		self.parser.update()

	#Dummy events below
	def TOUCH_DOWN(self, blobID):
		pass

	def TOUCH_UP(self, blobID, x, y):
		pass

	def TOUCH_MOVE(self, blobID):
		pass

	def FSEQ(self, framenum):
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
