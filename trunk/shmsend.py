#!/usr/bin/env python
#example use: shmsend.py 100 200 300 400
#shm.remove_memory(mem.shmid)
import shm
import sys
import ctypes

class tuio(ctypes.Structure):
	_fields_ = [
		('x',ctypes.c_int),
		('y',ctypes.c_int),
		('id',ctypes.c_int)
	]
class nested(ctypes.Structure):
	_fields_=[
		('comm',tuio),
		('alive',ctypes.c_int*20)
	]

x = int(sys.argv[1])
y = int(sys.argv[2])

key = 1972
mem = shm.memory(shm.getshmid(key))
sem = shm.semaphore(shm.getsemid(key))
sem.setval(0)
mem.attach()
tuio_cmd = nested()

tuio_cmd.comm.x=x
tuio_cmd.comm.y=y
tuio_cmd.comm.id=2
tuio_cmd.alive[0]=1
tuio_cmd.alive[1]=2
tuio_cmd.alive[2]=3

sem.Z()
mem.write(tuio_cmd)
sem.setval(2)
print "Sent:",x,y;
