Few lines about the project

# Introduction #
touchpy is a python framework to develop media rich
apps with multitouch capabilities.  This supports any screen(DIY or Commercial), using any method(DI, FTIR...), with any source of OSC coming at it(Touchlib, ReactABLE, TUIO Simulator).

# Details #

touchpy is still in early development, but its showing the possibilities
of combining the multitouch screens with Compiz Fusion and the GNU/Linux environment.
For now it's developed on a Linux platform but should be fairly easy to port to other environments that support X11 and Python.

Right now the code is still a mess, and the python part will be rewriten
from scrach. As we need events fired on blobIds instead on simple touch events.
That was because of one dimensional thinking, as I didn't think of multiple pointers. :)
Anyway the code is still usable, but the problem is that events get fired on every blob
and within the objects receiving that event, there is a need to test if the blob that is producing it is the right one.

Documentation will be written as the projects gets some alpha/beta release.

Video of the compiz plugin with touchpy in action:
http://video.google.com/videoplay?docid=5117324530629018540&hl=en

Moving windows with touchpy
http://video.google.com/videoplay?docid=-6433132392970567985&hl=en