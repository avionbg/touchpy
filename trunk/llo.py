import liblo

class LibloParser(object):
    address = "/tuio/2Dcur"
    def __init__(self, callback, host='127.0.0.1', port=3333):
      self.callback = callback
      try:
        self.server = liblo.Server(port)
      except liblo.ServerError, err:
        sys.exit(str(err))
      self.server.add_method(self.address, None, callback)

    def update(self):
        self.server.recv(0)
