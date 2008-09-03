#! /usr/bin/python -u
import pygame,sys
from pygame.locals import *

#delete line below if you have touchpy installed in your pythonpath
sys.path = ['..'] + sys.path

from framework import *

def main():
	# init pygame
	t = touchframework(None,None,800,600)

	img = pygame.Surface([40,40])
	img = img.convert()
	img.fill((0xff, 0xff, 0xff))
	img.set_colorkey((0xff, 0xff, 0xff), RLEACCEL)
	pygame.draw.line(img, (0,0,255), (20,0), (20,40), 3)
	pygame.draw.line(img, (0,0,255), (0,20), (40,20), 3)
	hero = pygame.sprite.Sprite()
	hero.image = img
	hero.rect = img.get_rect()
	hero.rect.centerx = 150
	hero.rect.centery = 150
	# you can use either array_colorkey() or array_alpha() as hitmask
	hero.hitmask = pygame.surfarray.array_colorkey(img)
	hero_group = pygame.sprite.RenderPlain(hero)
	# hero.touchdown = pp
	# hero.touchmove = pp
	# hero.touchup = pp
	t.register(hero)

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
			t.update()
			# clear screen
			t.screen.fill((255,255,255))           
			hero_group.draw(t.screen)
	except (KeyboardInterrupt, SystemExit):
		del t
 
if __name__ == '__main__': main()
