#! /usr/bin/python -u
'''
This is a Skeleton file.  It provides a 
template to be used when writing a new
TouchPy application.
'''

from touch import *


class Observer(object):
	def __init__(self, subject):
		subject.push_handlers(self)

class touch_up(Observer):
	def TOUCH_UP(self,blobID, xpos, ypos):
		#Do something here

class touch_down(Observer):
	def TOUCH_DOWN(self,blobID):
		#Do something here

class touch_move(Observer):
	def TOUCH_MOVE(self,blobID):
		#Do something here

t = touchpy()
tu = touch_up(t)
td = touch_down(t)
tm = touch_move(t)

try:
	while True:
		t.update()

except (KeyboardInterrupt, SystemExit):
	del t
