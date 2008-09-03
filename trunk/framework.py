from touch import *

def contains(obj, x, y):
	'''Return boolean whether the point defined by x, y is inside the
	rect area.
	'''
	if x < obj._x or x > obj._x + obj._width: return False
	if y < obj._y or y > obj._y + obj._height: return False
	return True

class touchframework (touchpy):
	def register(self, thing):
		if thing == None:
			del self.things[thing]
		else:
			self.things[thing] = thing
	def test_under (self,blobId,things):
		for thing in things:
			if contains(thing,x, y):
				blobId.attach(thing)
				self.things[thing].touchdown()
	def handle2Dcur(self, path, args, types, src):
		if args[0] == 'alive':
			touch_release = difference(self.alive,args[1:])
			#touch_down = difference(self.alive,args[1:])
			#touch_move = intersection(self.alive,args[1:])
			self.alive = args[1:]
			for blobID in touch_release:
				self.dispatch_event('TOUCH_UP', blobID, self.blobs[blobID].xpos, self.blobs[blobID].ypos)
				if self.blobs[blobID].thing: self.blobs[blobID].thing.touchup()
				del self.blobs[blobID]

		elif args[0] == 'set':
			blobID = args[1]
			if blobID not in self.blobs:
				self.blobs[blobID] = self.cursorparser(blobID,args[2:])
				self.dispatch_event('TOUCH_DOWN', blobID)
				self.test_under(self.blobs[blobID],self.things)
			else:
				self.blobs[blobID].move(args[2:])
				self.dispatch_event('TOUCH_MOVE', blobID)
				if self.blobs[blobID].thing: self.blobs[blobID].thing.touchmove()

		elif args[0] == 'fseq':
			self.last_frame = self.current_frame
			self.current_frame = args[1]
			self.dispatch_event('FSEQ', self.current_frame)


