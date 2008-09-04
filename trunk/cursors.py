import event

class Generic2DCursor(event.EventDispatcher):
	def __init__(self, blobID,args):
		self.blobID = blobID
		self.oxpos = self.oypos = 0.0
		if len(args) == 5:
			self.xpos, self.ypos, self.xmot, self.ymot, self.mot_accel = args[0:5]
		else:
			self.xpos, self.ypos, self.xmot, self.ymot, self.mot_accel, self.Width , self.Height = args[0:7]

	def move(self, args):
		self.oxpos, self.oypos = self.xpos, self.ypos
		if len(args) == 5:
			self.xpos, self.ypos, self.xmot, self.ymot, self.mot_accel = args[0:5]
		else:
			self.xpos, self.ypos, self.xmot, self.ymot, self.mot_accel, self.Width , self.Height = args[0:7]

class Touch2DCursor(event.EventDispatcher):
	def __init__(self, blobID,args):
		self.blobID = blobID
		self.oxpos = self.oypos = 0.0
		self.xpos, self.ypos, self.xmot, self.ymot, self.mot_accel, self.Width , self.Height = args[0:7]

	def move(self, args):
		self.oxpos, self.oypos = self.xpos, self.ypos
		self.xpos, self.ypos, self.xmot, self.ymot, self.mot_accel, self.Width , self.Height = args[0:7]

class exTouch2DCursor(Touch2DCursor):
	def __init__(self,blobID,args):
		super(exTouch2DCursor,self).__init__(blobID,args)
		self.sprite = None
	def attach(self, sprite):
		self.sprite = sprite

class Simul2DCursor(event.EventDispatcher):
	def __init__(self, blobID,args):
		self.blobID = blobID
		self.oxpos = self.oypos = 0.0
		self.xpos, self.ypos, self.xmot, self.ymot, self.mot_accel = args[0:5]

	def move(self, args):
		self.oxpos, self.oypos = self.xpos, self.ypos
		self.xpos, self.ypos, self.xmot, self.ymot, self.mot_accel = args[0:5]

class exSimul2DCursor(Simul2DCursor):
	def __init__(self,blobID,args):
		super(exSimul2DCursor,self).__init__(blobID,args)
		self.sprite = None
	def attach(self, sprite):
		self.sprite = sprite
