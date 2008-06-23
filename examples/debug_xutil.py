#!/usr/bin/python -u
#import wutil
from touch import *

#try:
import xutil
xutil.initXGraphics()
if xutil.getWindowID() == -1:
	raise EnvironmentError
from xutil import *
hasXWindow = 1
print xutil.getdisplaysize()
closeGraphics = closeXGraphics
del xutil
#except ImportError:
#	hasXWindow = 0 # Unsuccessful init of XWindow
#	print "importerror"
#except EnvironmentError:
#	print "enviroment error"
#	hasXWindow = 0 # Unsuccessful init of XWindow


