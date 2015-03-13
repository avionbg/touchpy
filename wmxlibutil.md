#Python extension to handle xlib calls

# Little python extension to xlib #

wmxlibutil is a collection of simple calls to xlib,
which simplify some of other difficult tasks.
There is (as probably all of you know) a native python xlib extension
but I found it rather difficult, and on other side some of the things
couldn't be done (well actually I'm not sure on this either, but for me
was easier to do C implementation)

# Calls Implemented #

  * getdisplaysize()
  * getWindowID()
  * moveCursorTo(WindowID, x, y)
  * window\_move(WindowID)
  * window\_setsize(WindowID, x, y)
  * mouse\_click(button)
  * setFocusTo(WindowID)
  * setBackingStore(WindowID)
  * getPointerPostion(WindowID)
  * getWindowAttributes(WindowID)
  * getParentID(WindowID)
  * getDeepestVisual() _returns\_always\_24_
  * initGraphics() _this is used by the wmxlibutil wrapper to initiate Display
  * closeGraphics()_same as initGraphic but closes the display
  * getWindowUnderCursor()
  * pointer2wid(xpoint,ypoint)

# example use: #
```
import wmxlibutil
(width,height) = wmxlibutil.getdisplaysize()
print "Display size is:",width,height
```

**note that the API and the naming will be probably changed**