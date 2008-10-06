import event

class Generic2DCursor(event.EventDispatcher):
	"""Generic cursor parsing class, which isn't actually used any more"""
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
	"""Touchlib format cursor parser"""
	def __init__(self, blobID,args):
		self.blobID = blobID
		self.oxpos = self.oypos = 0.0
		self.xpos, self.ypos, self.xmot, self.ymot, self.mot_accel, self.Width , self.Height = args[0:7]

	def move(self, args):
		"""Cursor already exist on this point, so we are just updating old x,y"""
		self.oxpos, self.oypos = self.xpos, self.ypos
		self.xpos, self.ypos, self.xmot, self.ymot, self.mot_accel, self.Width , self.Height = args[0:7]
		self.motcalc()

	#Calculates the relative movement, incase tracker is not providing it(in the case of touchlib)
	def motcalc(self):
		self.xmot = self.xpos - self.oxpos
		self.ymot = self.ypos - self.oypos

class exTouch2DCursor(Touch2DCursor):
	"""Subclass of touchlib cursor for framework use,
	extended to hold sprite object so we can know what was pressed"""
	def __init__(self,blobID,args):
		super(exTouch2DCursor,self).__init__(blobID,args)
		self.sprite = None
	def attach(self, sprite):
		self.sprite = sprite

class Simul2DCursor(event.EventDispatcher):
	"""Reactivision format cursor parser"""
	def __init__(self, blobID,args):
		self.blobID = blobID
		self.oxpos = self.oypos = 0.0
		self.xpos, self.ypos, self.xmot, self.ymot, self.mot_accel = args[0:5]

	def move(self, args):
		self.oxpos, self.oypos = self.xpos, self.ypos
		self.xpos, self.ypos, self.xmot, self.ymot, self.mot_accel = args[0:5]

class exSimul2DCursor(event.EventDispatcher):
	"""Extended version of reactivision cursor for framework use"""
	def __init__(self,blobID,width,height,args):
		self.width = width
		self.height = height
		self.sprite = None
		self.circle = None
		self.blobID = blobID
		self.oxpos = self.oypos = 0.0
		self.xpos = int(args[0] * width)
		self.ypos = int(args[1] * height)
		self.xmot, self.ymot, self.mot_accel = args[2:5]
	def move(self, args):
		self.oxpos, self.oypos = self.xpos, self.ypos
		self.xpos = int(args[0] * self.width)
		self.ypos = int(args[1] * self.height)
		self.xmot, self.ymot, self.mot_accel = args[2:5]

	def attach(self, sprite):
		self.sprite = sprite
