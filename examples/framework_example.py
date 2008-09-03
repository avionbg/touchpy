#! /usr/bin/python -u
import pygame,sys
from pygame.locals import *

#delete line below if you have touchpy installed in your pythonpath
sys.path = ['..'] + sys.path

from framework import *

def main():
	# init pygame
	pygame.init()    
	clock = pygame.time.Clock()

	# setup the display
	screen = pygame.display.set_mode((600, 600),1)
	pygame.display.set_caption('Touchpy V2 is becoming real multimedia framework!')        

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
	hero.touchdown = pp
	hero.touchmove = pp
	hero.touchup = pp
	t = touchframework()
	t.register(hero)

	try:
		while True:
			clock.tick(30)        
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
			screen.fill((255,255,255))           
			ticks = pygame.time.get_ticks()        
			time = pygame.time.get_ticks()-ticks
			hero_group.draw(screen)
			pygame.display.flip()
	except (KeyboardInterrupt, SystemExit):
		del t
 
if __name__ == '__main__': main()
