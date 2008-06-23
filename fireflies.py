#!/usr/bin/python -u
"""
Example of touchpy in combination with pygame and rabbyt
try to press and hold space key to see something ;)
"""
import random

import rabbyt
from rabbyt import lerp, wrap
import pygame.time, math
from touch import *
from pygame.locals import*
import os.path
rabbyt.data_directory = os.path.dirname(__file__)

pygame.init()
pygame.display.set_mode((640, 480), pygame.OPENGL | pygame.DOUBLEBUF)# | pygame.HWSURFACE | pygame.FULLSCREEN )
rabbyt.set_viewport((640, 480))
rabbyt.set_default_attribs()

sprites = set()

        
r = lambda: random.random()-.5
r2 = lambda: random.choice((-1,1))

class Particle(rabbyt.Sprite):
    width = 12
    radius = 80
    def __init__(self, coords, form_center, fussion_centar):
        rabbyt.Sprite.__init__(self, 'star.png')
        self.updt = 0
        self.coords = coords
        self.form_center = form_center
        self.fussion_center = fussion_centar
        self.pos = random.choice(range(len(coords)))
        self.xy = self.fussion_center[0]+self.radius*r()*2*3.14*r(),\
                  self.fussion_center[1]+self.radius*r()*2*3.14*r()
        self.rx, self.ry = self.fussion()
        self.dt = 800+400*r()
        self.form = False
        self.x = lerp(self.rx[0],self.rx[1], dt = self.dt)
        self.y = lerp(self.ry[0],self.ry[1], dt = self.dt)


        
        self.scale = lerp(0.1, 0.7, dt=self.dt, extend="reverse")
        self.rgba = lerp((1.0,0.9,0.1,0.9), (1.0,1.0,1.0,0.2), dt=self.dt, extend="reverse")

    def fussion(self):
        '''Kada ne radi nista vec se pomera bez cilja tj. zuji :)'''
        x = self.radius*r()*2*3.14*r()+self.fussion_center[0]
        y = self.radius*r()*2*3.14*r()+self.fussion_center[1]
        rx = self.x, x
        ry = self.y, y
        return rx, ry
    def make_form(self):
        '''formira oblik od kordinata'''
        past_xy=self.coords[self.pos]
        direction = r2()
        try:
            if self.pos == 0:
                if direction == -1:
                    direction = 1
            new_xy = self.coords[self.pos+direction]
            rx = (self.x,
                  new_xy[0]+r()*self.width+self.form_center[0]
                  )
            ry = (self.y,
                  new_xy[1]+r()*self.width+self.form_center[1]
                  )
        except:
            direction*=-1
            if self.pos == 0:
                if direction == -1:
                    direction = 1
            new_xy = self.coords[self.pos+direction]
            rx = (self.x,
                  new_xy[0]+r()*self.width+self.form_center[0]
                  )
            ry = (self.y,
                  new_xy[1]+r()*self.width+self.form_center[1]
                  )
        self.pos+=direction
        return rx, ry
    def update(self):
        if pygame.time.get_ticks() - self.updt > self.dt:
            self.updt = pygame.time.get_ticks()
            self.dt = 800+400*r()
            if self.form:
                self.rx, self.ry = self.make_form()
            else:
                self.rx, self.ry = self.fussion()

            self.x = lerp(self.rx[0],self.rx[1], dt = self.dt)
            self.y = lerp(self.ry[0],self.ry[1], dt = self.dt)
           
class ParticleCommander:
    def __init__(self, shape, shape_pos, position = (0,0), number = 300):
        self.xy = position
        self.w = shape_pos[0]-position[0]
        self.h = shape_pos[1]-position[1]
        self.particles =set()
        for x in xrange(number):
            self.particles.add(Particle(shape, shape_pos, position))
    def render(self):
        for p in self.particles:
            p.fussion_center = self.xy
            p.form_center = self.w+p.fussion_center[0],self.h+p.fussion_center[1]
            
            p.update()
        rabbyt.render_unsorted(self.particles)
        
t = touchpy()
w_letter = [(-35, 29), (-18, -34), (-1, -2), (16, -31), (31, 32)]
o_letter = []


for a in xrange(0,360,60):
    a = math.radians(a)
    o_letter.append((math.sin(a)*27,math.cos(a)*35))



o_letter.append(o_letter[0])

p_w1 = ParticleCommander(w_letter,(-70,0), number = 70)
p_w2 = ParticleCommander(w_letter,(70,0), number = 70)
p_o = ParticleCommander(o_letter,(0,0),number = 50)
c = pygame.time.Clock()
last_fps = 0
x,y = 0,0

class Observer(object):
	def __init__(self, subject):
		subject.push_handlers(self)

class touch_up(Observer):
	def TOUCH_UP(self,blobID, xpos, ypos):
		print 'blob release detected: ', blobID, xpos, ypos
		pass

class touch_down(Observer):
	def TOUCH_DOWN(self,blobID):
		global p_w1,p_w2,p_o
		x = int(t.blobs[blobID].xpos*480 -320)
		y = int(240 - t.blobs[blobID].ypos *640)
		for p in (p_w1,p_w2,p_o):
			p.xy = x,y
		rabbyt.clear()
		rabbyt.set_time(pygame.time.get_ticks())
		for p in (p_w1,p_w2,p_o):
			p.render()
		pygame.display.flip()
        pass

class touch_move(Observer):
	def TOUCH_MOVE(self,blobID):
		global p_w1,p_w2,p_o
		x = int(t.blobs[blobID].xpos*480 -320)
		y = int(240 - t.blobs[blobID].ypos *640)
		for p in (p_w1,p_w2,p_o):
			p.xy = x,y
		rabbyt.clear()
		rabbyt.set_time(pygame.time.get_ticks())
		for p in (p_w1,p_w2,p_o):
			p.render()
		pygame.display.flip()
        pass


td = touch_down(t)
tm = touch_move(t)
while not pygame.event.get(pygame.QUIT):
    c.tick()
    t.update()
    if pygame.time.get_ticks() - last_fps > 1000:
        #print "FPS: ", c.get_fps()
        last_fps = pygame.time.get_ticks()
    event = pygame.event.poll()
    keystate =pygame.key.get_pressed()
    mstate = pygame.mouse.get_pressed()
    if mstate[0]:
        mpos = pygame.mouse.get_pos()
        x = mpos[0]-320
        y = 240-mpos[1]
        for p in (p_w1,p_w2,p_o):
            p.xy = x,y
    if keystate[K_SPACE]:
        for p in (p_w1,p_w2,p_o):
            for pi in p.particles:
                pi.form = 1
    elif keystate[K_ESCAPE] or pygame.event.peek(QUIT):
        break
    else:
        for p in (p_w1,p_w2,p_o):
            for pi in p.particles:
                pi.form = 0        

    rabbyt.clear()
    rabbyt.set_time(pygame.time.get_ticks())
    for p in (p_w1,p_w2,p_o):
        p.render()
    pygame.display.flip()
