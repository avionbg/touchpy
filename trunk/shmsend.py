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
		('xo',ctypes.c_int),
		('yo',ctypes.c_int),
		('id',ctypes.c_int)
	]
class nested(ctypes.Structure):
	_fields_=[
		('lock',ctypes.c_int),
		('comm',tuio*20)
	]

x = int(sys.argv[1])
y = int(sys.argv[2])
if len(sys.argv) >3:
	xo = int(sys.argv[3])
	yo = int(sys.argv[4])

key = 5679 #1972
mem = shm.memory(shm.getshmid(key))
sem = shm.semaphore(shm.getsemid(key))
sem.setval(0)
#sem = shm.create_semaphore(key, 0)
mem.attach()
tuio_cmd = nested()
i=0
if len(sys.argv) >3:
	#for i in range(11):
		#an_array[i].x = x
		#an_array[i].y = y
		#an_array[i].xo = i
		#an_array[i].yo = i
		#an_array[i].lock = 1
	tuio_cmd.comm[i].x=x
	tuio_cmd.comm[i].y=y
	tuio_cmd.comm[i].xo=xo
	tuio_cmd.comm[i].yo=yo
	tuio_cmd.comm[i].id=1

	tuio_cmd.comm[1].x=x+100
	tuio_cmd.comm[1].y=y+100
	tuio_cmd.comm[1].xo=xo+100
	tuio_cmd.comm[1].yo=yo+100
	tuio_cmd.comm[1].id=2

	tuio_cmd.comm[2].x=x+300
	tuio_cmd.comm[2].y=y+300
	tuio_cmd.comm[2].xo=xo+300
	tuio_cmd.comm[2].yo=yo+300
	tuio_cmd.comm[2].id=2
	#tuio_cmd.lock=1
        sem.Z()
	mem.write(tuio_cmd)
	sem.setval(2)
	print "Sent:",x,y,xo,yo;
#else:
	#for i in range(20):
			#an_array[i].x = i
			#an_array[i].y = i
			#an_array[i].lock = 1
	#mem.write(an_array)
	#print "Sent:",x,y;
