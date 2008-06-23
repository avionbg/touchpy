#!/usr/bin/python -u
"""
Example of touchpy in combination with pygame and rabbyt
here are two examples, sparkles (fire) and smoke .. to have
latest you need to change lines 72 and 79 to reflect that (smoke) ;)
"""
import random

import rabbyt
from rabbyt import lerp, wrap
from pygame.locals import*
import pygame
import os.path
from touch import *

pygame.init()
pygame.display.set_mode((640, 480), pygame.OPENGL | pygame.DOUBLEBUF)
rabbyt.set_viewport((640, 480))
rabbyt.set_default_attribs()

sprites = set()
t = touchpy()

r = lambda: random.random()-.5
ran = lambda:random.random()*5

def get_xy(x,y):
    return x-320, 240-y


def fire(xy):
    s = rabbyt.Sprite('sparkles.png')
    s.xy = xy
    lifetime = 700
    s.rgba = lerp((1.0,1.0,0.0,1.0),(1.0,0.0,0.0,0.0), dt = 400)		
    #s.scale = lerp(1.0,0.0, dt = lifetime)
    s.scale = r()*5
    s.rot  = ran()
    s.x = lerp(s.x, r()*150+s.x, dt = lifetime)
    #s.y = lerp(s.y, r()*150+s.y, dt = lifetime)
    s.y = rabbyt.ease_in(s.y, ran()*-150+s.y, dt = lifetime)
    sprites.add(s)
    rabbyt.scheduler.add(rabbyt.get_time()+lifetime,
                lambda:sprites.remove(s))

def smoke(xy):
    s = rabbyt.Sprite('smoke.png')
    s.xy = xy
    lifetime = 900
    s.rgba = lerp((0.6,0.6,0.6,0.7),(0.3,0.3,0.3,0.0), dt = lifetime)		
    s.scale = lerp(0.5,1, dt = lifetime)
    s.rot  = r()*360
    s.x = lerp(s.x, r()*200+s.x, dt = lifetime)
    s.y = lerp(s.y, r()*200+s.y, dt = lifetime)
    sprites.add(s)
    rabbyt.scheduler.add(rabbyt.get_time()+lifetime,
                lambda:sprites.remove(s))

class Observer(object):
	def __init__(self, subject):
		subject.push_handlers(self)

class touch_up(Observer):
	def TOUCH_UP(self,blobID, xpos, ypos):
		print 'blob release detected: ', blobID, xpos, ypos
		pass

class touch_down(Observer):
	def TOUCH_DOWN(self,blobID):
		x = int(t.blobs[blobID].xpos*480 -320)
		y = int(240 - t.blobs[blobID].ypos *640)
		fire((x,y))
        pass

class touch_move(Observer):
	def TOUCH_MOVE(self,blobID):
		x = int(t.blobs[blobID].xpos*480 -320)
		y = int(240 - t.blobs[blobID].ypos *640)
		fire((x,y))
        pass

c = pygame.time.Clock()
last_fps = 0

td = touch_down(t)
tm = touch_move(t)

while not pygame.event.get(pygame.QUIT):
    c.tick()
    t.update()
    event = pygame.event.poll()
    if event.type == KEYDOWN:
        if event.key == K_ESCAPE:
            break
    mstate = pygame.mouse.get_pressed()
    mpos = pygame.mouse.get_pos()
    mpos = get_xy(mpos[0],mpos[1])
    #print mpos

    if mstate[0]:
	#for x in xrange(2):
        fire(mpos)
    if mstate[2]:
	smoke(mpos)        
    if pygame.time.get_ticks() - last_fps > 1000:
        #print "FPS: ", c.get_fps()
        last_fps = pygame.time.get_ticks()
        #print len(sprites)
	#This is needed to have constant particles when holding finger on surface
    for blob in t.blobs:
		x = int(t.blobs[blob].xpos*480 -320)
		y = int(240 - t.blobs[blob].ypos *640)
		fire((x,y))

    rabbyt.scheduler.pump()
    rabbyt.clear()
    rabbyt.set_time(pygame.time.get_ticks())
    rabbyt.render_unsorted(sprites)
    pygame.display.flip()

pygame.quit()
