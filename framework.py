from touch import touchpy,difference
from cursors import exSimul2DCursor
import event,sys
import pygame

def contains(obj, x, y):
	'''Return boolean whether the point defined by x, y is inside the
	rect area.
	'''
	if x < obj._x or x > obj._x + obj._width: return False
	if y < obj._y or y > obj._y + obj._height: return False
	return True

class touchframework (touchpy):
	def __init__(self, host='127.0.0.1', port=3333, width=0, height=0):
		super(touchframework,self).__init__( host, port)
		self.sprites = {}
		pygame.init()    
		self.clock = pygame.time.Clock()
		if width == 0:
			if sys.platform == 'win32':
				from win32api import GetSystemMetrics
				(self.width,self.height) = GetSystemMetrics (0), GetSystemMetrics (1)
			elif sys.platform == 'linux':
				from wmxlibutil import getdisplaysize
				(self.width,self.height) = getdisplaysize()
		else:
			(self.width,self.height) = width,height
		self.screen = pygame.display.set_mode((self.width, self.height),1)
		pygame.display.set_caption('Touchpy V2 is becoming real multimedia framework!')        

	def register(self, sprite):
		if sprite == None:
			del self.sprites[sprite]
		else:
			self.sprites[sprite] = sprite

	def setup(self, path, args, types, src):
		if args[0] == 'set':
			if len(args[2:]) == 5:
				self.cursorparser = exSimul2DCursor
				self.provider = 'TUIOSimulator'
			else:
				self.cursorparser = exTouch2DCursor
				self.provider = 'Touchlib'
		elif args[0] == 'fseq' and hasattr(self, 'provider'):
			self.parser.subst(self.handle2Dcur)
		self.handle2Dcur(path, args, types, src)

	def test_under (self,blobId,sprites):
		for sprite in sprites:
			if contains(sprite,x, y):
				blobId.attach(thing)
				self.sprites[sprite].touchdown()

	def handle2Dcur(self, path, args, types, src):
		if args[0] == 'alive':
			touch_release = difference(self.alive,args[1:])
			self.alive = args[1:]
			for blobID in touch_release:
				self.dispatch_event('TOUCH_UP', blobID, self.blobs[blobID].xpos, self.blobs[blobID].ypos)
				print 'Touchup'
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

	def update(self):
		self.parser.update()
		self.clock.tick(30)
		ticks = pygame.time.get_ticks()        
		time = pygame.time.get_ticks()-ticks
		pygame.display.flip()

