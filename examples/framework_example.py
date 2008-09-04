#! /usr/bin/python -u
import pygame,sys
from pygame.locals import *

#delete line below if you have touchpy installed in your pythonpath
sys.path = ['..'] + sys.path

from framework import *

# Subclass pygame.sprite.Sprite
class Sprite(pygame.sprite.Sprite):
	def __init__(self):
		pygame.sprite.Sprite.__init__(self)
		self._x = 0
		self._y = 0
	def touchup(self):
		print 'touchup'
	def touchdown(self):
		print 'touchdown'
	def touchmove(self,x,y):
		self.rect.centerx = x
		self.rect.centery = y
		self.group.draw(self.screen)
		print 'touchmove',x,y
	def isgroup(self,group):
		self.group = group
	def isscreen(self,screen):
		self.screen = screen
def makeSprite(x,y):
	img = pygame.Surface([20,20])
	img = img.convert()
	img.fill((0xff, 0xff, 0xff))
	img.set_colorkey((0xff, 0xff, 0xff), RLEACCEL)
	pygame.draw.line(img, (255,0,0), (0,0), (19,19), 3)
	pygame.draw.line(img, (255,0,0), (0,19), (19,0), 3)
	#foo = pygame.sprite.Sprite()
	foo = Sprite()
	foo.image = img
	foo.rect = img.get_rect()
	foo.rect.centerx = x
	foo.rect.centery = y
	foo.hitmask = pygame.surfarray.array_colorkey(img)
	return foo

def main():
	# init pygame
	#t = touchframework()
	t = touchframework(None,None,800,600)

	sprite = makeSprite(20,20)
	sprites = pygame.sprite.RenderPlain()
	sprites.add(sprite)
	sprite.isgroup(sprites)
	sprite.isscreen(t.screen)
	t.register(sprite)

	try:
		while True:
			for event in pygame.event.get():
				if event.type == QUIT:
					del t
					return
				elif event.type == KEYUP:
					if event.key == K_ESCAPE:
						del t
						return
				elif event.type == MOUSEMOTION:
					if event.buttons[0]:
						sprite.rect.centerx = event.pos[0]   
						sprite.rect.centery = event.pos[1]   
			t.update()
			# clear screen
			t.screen.fill((255,255,255))
			sprites.draw(t.screen)
	except (KeyboardInterrupt, SystemExit):
		del t
 
if __name__ == '__main__': main()
