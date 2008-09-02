import OSC,socket

class RawParser(object):
	def __init__(self, callback, host='127.0.0.1', port=3333, address = "/tuio/2Dcur"):
		self.address = address
		self.callback = callback
		self.socket = socket.socket(socket.AF_INET, socket.SOCK_DGRAM)
		self.socket.setsockopt(socket.SOL_SOCKET, socket.SO_REUSEADDR, 1)
		self.socket.setblocking(0)
		self.socket.bind((host,port))
		self.rawosc = OSC.CallbackManager()
		self.rawosc.add(self.fallback, address)

	def fallback(self, *incoming):
		message = incoming[0]
		path, types, args = message[0], message[1], message[2:]
		if (path == self.address):
			self.callback(path,args,types,"raw")

	def subst(self, callback):
		self.callback = callback

	def update(self):
		try:
			self.rawosc.handle(self.socket.recv(1024))
		except socket.error:
			pass

	def __del__(self):
		self.socket.close()
