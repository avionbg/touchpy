#! /usr/bin/python -u
import pygame,sys
from pygame.locals import *

#delete line below if you have touchpy installed in your pythonpath
sys.path = ['..'] + sys.path

from framework import *

def makeSprite(x,y):
	img = pygame.Surface([40,40])
	img = img.convert()
	img.fill((0xff, 0xff, 0xff))
	img.set_colorkey((0xff, 0xff, 0xff), RLEACCEL)
	pygame.draw.line(img, (255,0,0), (0,0), (39,39), 3)
	pygame.draw.line(img, (255,0,0), (0,39), (39,0), 3)
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
			# clear screen
			t.screen.fill((255,255,255))
			sprites.draw(t.screen)
			t.update()
	except (KeyboardInterrupt, SystemExit):
		del t
 
if __name__ == '__main__': main()
