from touch import touchpy,difference
from cursors import exSimul2DCursor,exTouch2DCursor
import event,sys
import pygame
from pygame.locals import *

def contains(sprite, x, y):
	'''Return boolean whether the point defined by x, y is inside the
	rect area.
	'''
	if x < sprite.rect.x or x > sprite.rect.x + sprite.rect.width: return False
	if y < sprite.rect.i or y > sprite.rect.y + sprite.rect.height: return False
	return True

def get_angle():#_between(a, b):
	a=vect.Vector((100,100),(100,500))
	b=vect.Vector((100,100),(100,800))
	print 'gotovo', a.angle_in_degrees(b)

def makeCircle(x,y):
	img = pygame.Surface([20,20])
	img = img.convert()
	img.fill((0xff, 0xff, 0xff))
	img.set_colorkey((0xff, 0xff, 0xff), RLEACCEL)
	pygame.draw.circle(img, (0,0,0), (10,10), 10, 1)
	#foo = pygame.sprite.Sprite()
	foo = Sprite()
	foo.image = img
	foo.rect = img.get_rect()
	foo.rect.centerx = x
	foo.rect.centery = y
	foo.hitmask = pygame.surfarray.array_colorkey(img)
	return foo

class Sprite(pygame.sprite.Sprite):
	"""Subclass pygame.sprite.Sprite to hold some of our data and methods"""
	def __init__(self):
		pygame.sprite.Sprite.__init__(self)
		self.blobs = [] #list of attached blobs
	def touchup(self,blobID):
		self.blobs.remove(blobID)
		print 'touchup'
	def touchdown(self):
		print 'touchdown'
	def touchmove(self,x,y):
		self.rect.centerx = x
		self.rect.centery = y
		self.group.draw(self.screen)
		print 'touchmove',x,y
	def touchgesture(self,list):
		print 'list of moved blobs',list
	def isgroup(self,group):
		self.group = group
	def isscreen(self,screen):
		self.screen = screen
	def attach(self, blobID):
		self.blobs.append(blobID)

class touchframework (touchpy):
	"""Subclass of touchpy, overriding some methods and adding values to hold pygame internals"""
	def __init__(self, host='127.0.0.1', port=3333, width=0, height=0):
		super(touchframework,self).__init__( host, port)
		#get_angle()
		self.sprites = {}
		pygame.init()
		self.moving = []
		self.clock = pygame.time.Clock()
		self.blobsprites = pygame.sprite.RenderPlain()

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
		"""Register sprite to event processing"""
		if sprite == None:
			del self.sprites[sprite]
		else:
			self.sprites[sprite] = sprite

	def setup(self, path, args, types, src):
		"""Overriding setup method to use extended cursor parser"""
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

	def test_under (self,blobID,sprites):
		"""Test for sprites under current blob x,y position"""
		blob = self.blobs[blobID]
		for sprite in sprites:
			#if contains(sprite,int(self.blobs[blobId].xpos*width), int(self.blobs[blobId].xpos*width)):
			if sprite.rect.collidepoint(blob.xpos, blob.ypos):
				blob.attach(sprite)
				self.sprites[sprite].attach(blob.blobID)
				self.sprites[sprite].touchdown()
	def handle2Dcur(self, path, args, types, src):
		"""Overriden cursor handler to fire sprite touchup,touchdown and touchmove methods"""
		if args[0] == 'alive':
			touch_release = difference(self.alive,args[1:])
			self.alive = args[1:]
			for blobID in touch_release:
				self.dispatch_event('TOUCH_UP', blobID, self.blobs[blobID].xpos, self.blobs[blobID].ypos)
				if self.blobs[blobID].sprite: self.blobs[blobID].sprite.touchup(blobID)
				self.blobsprites.remove(self.blobs[blobID].circle)
				del self.blobs[blobID]
		elif args[0] == 'set':
			blobID = args[1]
			if blobID not in self.blobs:
				self.blobs[blobID] = self.cursorparser(blobID,self.width,self.height,args[2:])
				blob = self.blobs[blobID]
				self.dispatch_event('TOUCH_DOWN', blobID)
				self.test_under(blobID,self.sprites)
				blob.circle = makeCircle(blob.xpos,blob.ypos)
				self.blobsprites.add(blob.circle)
			else:
				blob = self.blobs[blobID]
				blob.move(args[2:])
				self.dispatch_event('TOUCH_MOVE', blobID)
				blob.circle.rect.centerx = blob.xpos
				blob.circle.rect.centery = blob.ypos
				if blob.sprite:
					self.moving.append(blobID)
		elif args[0] == 'fseq':
			self.last_frame = self.current_frame
			self.current_frame = args[1]
			self.dispatch_event('FSEQ', self.current_frame)
			for blobID in self.moving: #go trough list of all moved blobs
				blob = self.blobs[blobID]
				if len(blob.sprite.blobs) > 1: #If there are more than 1 blobs attached to sprite, call gesture with list of all blobs that has been moved this frame
					blob.sprite.touchgesture()
				else:
					blob.sprite.touchmove(blob.xpos, blob.ypos) #otherway call the move callback with new x,y position
				print blob.xpos,blob.ypos,len(blob.sprite.blobs),blob.sprite.blobs
			self.moving = [] #clear list of moving blobs (this is done to have a clean list for the next frame)
			self.blobsprites.draw(self.screen)
	def update(self):
		"""Here we added pygame updates"""
		self.parser.update()
		self.refreshscreen()
	def refreshscreen(self):
		self.blobsprites.draw(self.screen)
		self.clock.tick(60)
		ticks = pygame.time.get_ticks()        
		time = pygame.time.get_ticks()-ticks
		pygame.display.flip()
