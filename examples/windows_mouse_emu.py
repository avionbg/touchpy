#!/usr/bin/python -u
"""
example windows mouse emulation with touchpy
you will need win32 module found on sourceforge!
"""

import win32api
import win32con
from ctypes import windll
import time
from touch import *

def m_move(x,y):
    windll.user32.SetCursorPos(x,y)

def l_click(x="current", y="current"):
    if x == "current" and y == "current":
        win32api.mouse_event(win32con.MOUSEEVENTF_LEFTDOWN, 0, 0)
        time.sleep(0.05)
        win32api.mouse_event(win32con.MOUSEEVENTF_LEFTUP, 0, 0)
    else:
        win32api.mouse_event(win32con.MOUSEEVENTF_LEFTDOWN, x, y)
        time.sleep(0.05)
        win32api.mouse_event(win32con.MOUSEEVENTF_LEFTUP, x, y)

def r_click(x="current", y="current"):
    if x == "current" and y == "current":
        win32api.mouse_event(win32con.MOUSEEVENTF_RIGHTDOWN, 0, 0)
        time.sleep(0.05)
        win32api.mouse_event(win32con.MOUSEEVENTF_RIGHTUP, 0, 0)
    else:
        win32api.mouse_event(win32con.MOUSEEVENTF_RIGHTDOWN, x, y)
        time.sleep(0.05)
        win32api.mouse_event(win32con.MOUSEEVENTF_RIGHTUP, x, y)



class Observer(object):
	def __init__(self, subject):
		subject.push_handlers(self)

class touch_up(Observer):
	def TOUCH_UP(self,blobID, xpos, ypos):
		if DEBUG: print 'blob release detected: ', blobID, xpos, ypos

class touch_down(Observer):
	def TOUCH_DOWN(self,blobID):
		if DEBUG: print 'blob press detected: ', blobID, t.blobs[blobID].xpos, t.blobs[blobID].ypos

class touch_move(Observer):
	def TOUCH_MOVE(self,blobID):
		if DEBUG: print 'blob move detected: ', blobID, t.blobs[blobID].xpos, t.blobs[blobID].ypos
                m_move(int(t.blobs[blobID].xpos*width),int(t.blobs[blobID].ypos*height))


from win32api import GetSystemMetrics
(width,height) = GetSystemMetrics (0), GetSystemMetrics (1)

t = touchpy()
tu = touch_up(t)
td = touch_down(t)
tm = touch_move(t)
DEBUG = 1

try:
	while True:
		t.update()

except (KeyboardInterrupt, SystemExit):
	del t
