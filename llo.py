import liblo

class LibloParser(object):
	def __init__(self, callback, host='127.0.0.1', port=3333, address = "/tuio/2Dcur"):
		self.callback = callback
		try:
			self.server = liblo.Server(port)
			self.server.add_method(address, None, callback)
		except liblo.ServerError, err:
			sys.exit(str(err))

	def update(self):
		self.server.recv(0)
