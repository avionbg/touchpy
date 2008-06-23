import sys
import direct.directbase.DirectStart
from direct.actor import Actor
from direct.showbase import DirectObject
from touch import touchpy

class Game(DirectObject.DirectObject):
    angle = 0
    distance = 0
    global t
    t = touchpy()

    def __init__(self):
        self.environ = loader.loadModel("models/environment")
        self.environ.reparentTo(render)
        self.environ.setScale(0.25,0.25,0.25)
        #self.environ.setPos(-8,42,0)
        #self.panda = loader.loadModel("models/panda")
        #self.panda.reparentTo(render)
        #self.panda.setPos(0, 1000, -100)
        #self.panda.setScale(0.5, 0.5, 0.5)

        self.pandaActor = Actor.Actor("models/panda-model",{"walk":"models/panda-walk4"})
        self.pandaActor.setScale(0.005,0.005,0.005)
        self.pandaActor.reparentTo(render)
        self.pandaActor.loop("walk")
        self.accept('escape' , sys.exit )
        base.disableMouse()
        #base.camLens.setFar(10000)
        taskMgr.add(self.touchupdate, 'touchupdate')

    def spinCamera(self, direction):
        self.angle += direction * 1.0
        base.camera.setHpr(self.angle, 0, 0)

    def zoomCamera(self, direction):
        self.distance += direction * 1.0
        base.camera.setPos(0, self.distance, 10)

    def touchupdate(self,task):
        t.update()
        return task.cont

    @t.event
    def TOUCH_DOWN(blobID):
        print 'blob press detected: ', blobID, t.blobs[blobID].xpos, t.blobs[blobID].ypos

    @t.event
    def TOUCH_UP(blobID,x,y):
        print 'blob release detected: ', blobID, x, y

    @t.event
    def TOUCH_MOVE(blobID):
        global game
        print 'blob move detected: ', blobID, t.blobs[blobID].xpos, t.blobs[blobID].ypos
        if t.blobs[blobID].xpos < t.blobs[blobID].oxpos:
            game.spinCamera(-1)
        elif t.blobs[blobID].xpos > t.blobs[blobID].oxpos:
            game.spinCamera(1)
        if t.blobs[blobID].ypos < t.blobs[blobID].oypos:
            game.zoomCamera(1)
        elif t.blobs[blobID].ypos > t.blobs[blobID].oypos:
            game.zoomCamera(-1)

game = Game()
run()
