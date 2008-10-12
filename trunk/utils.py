'''Touchpy Utils file'''

'''So far all it can do is give you the screen size, but more functionality 
may be added in the future.'''

import os
import sys

def _get_sz_linux2():
    sz = os.popen('xrandr | grep current').read()
    sz = sz[sz.index('current') + 8:]
    sz = sz[:sz.index(',')].split(' x ')
    sz = [int(x) for x in sz]
    return sz

def _get_sz_darwin():
	sz = os.popen("osascript -e 'tell application \"Finder\" to get bounds of window of desktop'").read()
	sz = sz.split(', ')
	sz[3] = sz[3][:-1]
	sz[2] = int(sz[2])
	sz[3] = int(sz[3])
	return (sz[2], sz[3])
 
#TODO: Test   
def _get_sz_nt():
    from win32api import GetSystemMetrics
    return (GetSystemMetrics(0), GetSystemMetrics(1))

def get_sz():
    if sys.platform == 'linux2':
        return _get_sz_linux2()
    elif sys.platform == 'darwin':
        return _get_sz_darwin()
    elif sys.platform == 'win32':
        return _get_sz_nt()
