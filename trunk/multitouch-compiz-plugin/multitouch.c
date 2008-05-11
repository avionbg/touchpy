/*
 * Multitouch input plugin
 * multitouch.c
 *
 * Author : Goran Medakovic
 * Email : avion.bg@gmail.com
 *
 * Copyright (c) 2008 Goran Medakovic <goran.it@gmail.com>
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License
 * as published by the Free Software Foundation; either version 2
 * of the License, or (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin Street, Fifth Floor,
 * Boston, MA  02110-1301, USA.
 */

#define KEY (5679)
#define MAXBLOBS (20)

#include <compiz-core.h>
#include "multitouch_options.h"

#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <math.h>

#include <sys/types.h>
#include <sys/ipc.h>
#include <sys/shm.h>
#include <sys/sem.h>
#include <pthread.h>

static int displayPrivateIndex;
// static int x, y;
// static CompScreen *s = NULL;

typedef struct
{
  int x, y, xo, yo, id;
} command;

typedef struct
{
  int lock;
  command comm[MAXBLOBS];
} tcommand;

typedef struct _MultitouchDisplay
{
  int screenPrivateIndex;

  pthread_t eventsThread;
  pthread_mutex_t lock;
  Bool endThread;
  Bool enabled;

  int tuio_ptr;

  CompTimeoutHandle timeoutHandles;
} MultitouchDisplay;

typedef struct _MultitouchScreen
{
  int windowPrivateIndex;
  Bool title;

} MultitouchScreen;


// This struct is used as closure for timeouts
typedef struct _DisplayValue
{
  CompDisplay *display;
  int x0;
  int y0;
  int x1;
  int y1;
} DisplayValue;

#define GET_MULTITOUCH_DISPLAY(d) \
((MultitouchDisplay *) (d)->base.privates[displayPrivateIndex].ptr)
#define MULTITOUCH_DISPLAY(d) \
MultitouchDisplay *md = GET_MULTITOUCH_DISPLAY (d)
#define GET_MULTITOUCH_SCREEN(s, md) \
((MultitouchScreen *) (s)->base.privates[(md)->screenPrivateIndex].ptr)
#define MULTITOUCH_SCREEN(s) \
MultitouchScreen *ms = GET_MULTITOUCH_SCREEN (s, GET_MULTITOUCH_DISPLAY (s->display))

/* TODO: Add raw tuio protocol parsing, toggling the effects trough key/gestures
	draw debug info on cairo, animated effect for each blob, pie (radial) menu

multitouchInitiate (CompDisplay     *d,
		 CompAction      *action,
		 CompActionState cstate,
		 CompOption      *option,
		 int             nOption)
{
}

Bool
multitouchTerminate (CompDisplay     *d,
		 CompAction      *action,
		 CompActionState cstate,
		 CompOption      *option,
		 int             nOption)
{
}

Bool
multitouchSendInfo (CompDisplay     *d,
		 CompAction      *action,
		 CompActionState cstate,
		 CompOption      *option,
		 int             nOption)
{
}
*/

// Function to send commands to other plugins
static Bool
sendInfoToPlugin (CompDisplay * d, CompOption * argument, int nArgument,
		  char *pluginName, char *actionName)
{
  Bool pluginExists = FALSE;
  CompOption *options, *option;
  CompPlugin *p;
  CompObject *o;
  int nOptions;

  p = findActivePlugin (pluginName);
  o = compObjectFind (&core.base, COMP_OBJECT_TYPE_DISPLAY, NULL);

  if (!o)
    return FALSE;

  if (!p || !p->vTable->getObjectOptions)
    {
      compLogMessage (d, "multitouch", CompLogLevelError,
		      "Reporting plugin '%s' does not exist!", pluginName);
      return FALSE;
    }

  if (p && p->vTable->getObjectOptions)
    {
      options = (*p->vTable->getObjectOptions) (p, o, &nOptions);
      option = compFindOption (options, nOptions, actionName, 0);
      pluginExists = TRUE;

    }

  if (pluginExists)
    {
      if (option && option->value.action.initiate)
	{

	  (*option->value.action.initiate) (d,
					    &option->value.action,
					    0, argument, nArgument);
	}
      else
	{

	  compLogMessage (d, "multitouch", CompLogLevelError,
			  "Plugin '%s' does exist, but no option named '%s' was found!",
			  pluginName, actionName);
	  return FALSE;
	}
    }

  return TRUE;
}

static Bool
multitouchToggle (CompDisplay * d,
		  CompAction * action,
		  CompActionState state, CompOption * option, int nOption)
{

  MULTITOUCH_DISPLAY (d);

  md->enabled = !md->enabled;

  return FALSE;
}

// Calls to other plugins
static Bool
makeripple (void *data)
{
  DisplayValue *dv = (DisplayValue *) data;
  CompScreen *s;
  s = findScreenAtDisplay (dv->display, currentRoot);

  CompOption arg[6];
  int nArg = 0;

  arg[nArg].name = "root";
  arg[nArg].type = CompOptionTypeInt;
  arg[nArg].value.i = s->root;
  nArg++;

  arg[nArg].name = "x0";
  arg[nArg].type = CompOptionTypeInt;
  arg[nArg].value.i = dv->x0;
  nArg++;

  arg[nArg].name = "y0";
  arg[nArg].type = CompOptionTypeInt;
  arg[nArg].value.i = dv->y0;
  nArg++;

  arg[nArg].name = "x1";
  arg[nArg].type = CompOptionTypeInt;
  arg[nArg].value.i = dv->x1;
  nArg++;

  arg[nArg].name = "y1";
  arg[nArg].type = CompOptionTypeInt;
  arg[nArg].value.i = dv->y1;
  nArg++;

  arg[nArg].name = "amplitude";
  arg[nArg].type = CompOptionTypeFloat;
  arg[nArg].value.i = 0.5f;	// 0.25f za point,0.5f za line

  sendInfoToPlugin (dv->display, arg, nArg, "water", "line");
  free (dv);
  return FALSE;			// Return False so the timeout gets removed upon first callback
}

static Bool
makeannotate (void *data)
{
  DisplayValue *dv = (DisplayValue *) data;
  CompScreen *s;
  s = findScreenAtDisplay (dv->display, currentRoot);
  if (s)
  {
  CompOption arg[7];
  int nArg = 0;

  arg[nArg].name = "root";
  arg[nArg].type = CompOptionTypeInt;
  arg[nArg].value.i = s->root;
  nArg++;

  arg[nArg].name = "tool";
  arg[nArg].type = CompOptionTypeString;
  arg[nArg].value.s = "line";
  nArg++;

  arg[nArg].name = "x1";
  arg[nArg].type = CompOptionTypeFloat;
  arg[nArg].value.f = (float) dv->x0;
  nArg++;

  arg[nArg].name = "y1";
  arg[nArg].type = CompOptionTypeFloat;
  arg[nArg].value.f = (float) dv->y0;
  nArg++;

  arg[nArg].name = "x2";
  arg[nArg].type = CompOptionTypeFloat;
  arg[nArg].value.f = (float) dv->x1;
  nArg++;

  arg[nArg].name = "y2";
  arg[nArg].type = CompOptionTypeFloat;
  arg[nArg].value.f = (float) dv->y1;
  nArg++;

// TODO: Check what's the format of ComOptionTypeColor
// for black it would be something like
// int i; for (i = 0; i < 3 i++){ c[i] = 0; } c[3] = 255;
// CompOptionTypeColor c;
// unsigned short c[3];
// // c[0] = 0;
// // c[1] = 0;
// // c[2] = 0;
// // c[3] = 255;
// int i; for (i = 0; i < 3; i++){ c[i] = 0; } c[3] = 255;
//   arg[nArg].name = "fill_color";
//   arg[nArg].type = CompOptionTypeColor;
//   arg[nArg].value.c = c;

  arg[nArg].name = "line_width";
  arg[nArg].type = CompOptionTypeFloat;
  arg[nArg].value.f = 1.5f;

  sendInfoToPlugin (dv->display, arg, nArg, "annotate", "draw");
  }
  free (dv);
  return FALSE;			// Return False so the timeout gets removed upon first callback
}

static Bool
displaytext (void *data)
{
  DisplayValue *dv = (DisplayValue *) data;
  CompScreen *s;
  s = findScreenAtDisplay (dv->display, currentRoot);

  CompOption arg[4];
  int nArg = 0;

  arg[nArg].name = "window";
  arg[nArg].type = CompOptionTypeInt;
  arg[nArg].value.i = dv->display->activeWindow;
  nArg++;

  arg[nArg].name = "root";
  arg[nArg].type = CompOptionTypeInt;
  arg[nArg].value.i = s->root;
  nArg++;

  arg[nArg].name = "string";
  arg[nArg].type = CompOptionTypeString;
  arg[nArg].value.s = "Multitouch initiated succesfully!";
  nArg++;

  arg[nArg].name = "timeout";
  arg[nArg].type = CompOptionTypeInt;
  arg[nArg].value.i = 2000;

  sendInfoToPlugin (dv->display, arg, nArg, "prompt", "display_text");
  free (dv);
  return FALSE;			// Return False so the timeout gets removed upon first callback
}

// Thread which is used to read commands sent over
// shared memory and trigger the main thread trough timeouts
static Bool
multitouchThread (void *display)
{
  CompDisplay *d = (CompDisplay *) display;
  MULTITOUCH_DISPLAY (d);
  int shmid, semid, retval, i;
  tcommand *tcomm;
  struct sembuf operations[1];
  union semun
  {
    int val;
    struct semid_ds *buf;
    ushort *array;
  } argument;
  argument.val = 0;
// Shared memory init
  shmid = shmget (KEY, sizeof (*tcomm), 0666 | IPC_CREAT);
  if (shmid == -1)
    {
      compLogMessage (d, "multitouch", CompLogLevelFatal,
		      "Error in allocating shm (error code: %d).", shmid);
      return FALSE;
    }				//if
  tcomm = shmat (shmid, NULL, 0);
  if (tcomm == (void *) -1)
    {
      compLogMessage (d, "multitouch", CompLogLevelFatal,
		      "Error attaching memory(error code: %d).", tcomm);
      return FALSE;
    }				//if

// Semaphore init
  semid = semget (KEY, 1, 0666 | IPC_CREAT);
  if (semid < 0)
    {
      compLogMessage (d, "multitouch", CompLogLevelFatal,
		      "Cannot find semaphore, exiting.(error code: %d).",
		      semid);
      return FALSE;
    }
  if (semctl (semid, 0, SETVAL, argument) < 0)
    {
      compLogMessage (d, "multitouch", CompLogLevelFatal,
		      "Cannot set semaphore value.");
      return FALSE;
    }
// Thread loop
  do
    {
      operations[0].sem_num = 0;
      operations[0].sem_op = -1;
      operations[0].sem_flg = 0;
      retval = semop (semid, operations, 1);
//       md->tuio_ptr = tcomm;
// Lock the mutex
      pthread_mutex_lock (&md->lock);
// Check If we have end flag set
      if (md->endThread)
	{
// We have end flag set, exit from loop and join with main thread
	  pthread_mutex_unlock (&md->lock);
	  break;
	}
// Unlock the mutex
      pthread_mutex_unlock (&md->lock);
// Continue with standard processing
      for (i = 0; i < MAXBLOBS; ++i)
	{
	  command *p = &(tcomm->comm[i]);
// Check for blobID, If there isn't any, it's void so skip it   
	  if (p->id)
	    {
// Check if there is old x and y, it's enough to check just one
	      if (p->xo)
		{
		  printf ("num:%d,X:%d,Y:%d,Xo:%d,Yo:%d,Id:%d\n",
			  i, p->x, p->y, p->xo, p->yo, p->id);

		  DisplayValue *dv = malloc (sizeof (DisplayValue));
		  if (!dv)
		    return FALSE;
		  dv->display = d;
		  dv->x0 = p->x;
		  dv->y0 = p->y;
		  dv->x1 = p->xo;
		  dv->y1 = p->yo;
		  md->timeoutHandles = compAddTimeout (0, makeannotate, dv);
		}
	      else
		{
		  printf ("num:%d,X:%d,Y:%d,Id:%d\n",
			  i, p->x, p->y, p->id);
		  DisplayValue *dv = malloc (sizeof (DisplayValue));
		  if (!dv)
		    return FALSE;
		  dv->display = d;
		  dv->x0 = p->x;
		  dv->y0 = p->y;
		  dv->x1 = p->x;
		  dv->y1 = p->y;
		  md->timeoutHandles = compAddTimeout (0, makeannotate, dv);
		}
	    }			//if
	}			//for
      operations[0].sem_num = 0;
      operations[0].sem_op = -1;
      operations[0].sem_flg = 0;
      retval = semop (semid, operations, 1);
    }
  while (!tcomm->lock);

  if (semctl (semid, 0, IPC_RMID, 0) < 0)
    {
      compLogMessage (d, "multitouch", CompLogLevelFatal,
		      "Cannot delete semaphore.");
      return FALSE;
    }
  if (shmdt (tcomm) == -1)
    {
      compLogMessage (d, "multitouch", CompLogLevelFatal,
		      "Error detaching shared memory.");
      return FALSE;
    }				//if
  if (shmctl (shmid, IPC_RMID, 0) == -1)
    {
      compLogMessage (d, "multitouch", CompLogLevelFatal,
		      "Error: shmctl(IPC_RMID) failed.");
      return FALSE;
    }				//if
  return (int) NULL;
}

static Bool
multitouchInitThread (CompDisplay * d)
{
  int status;
  MULTITOUCH_DISPLAY (d);

  pthread_attr_t attr;
  pthread_attr_init (&attr);
  pthread_attr_setdetachstate (&attr, PTHREAD_CREATE_JOINABLE);
  status =
      pthread_create (&md->eventsThread, &attr, multitouchThread,
		      (void *) d);
  pthread_attr_destroy (&attr);
  if (status)
    {
      compLogMessage (d, "multitouch", CompLogLevelFatal,
		      "Could not start thread (error code: %d).", status);
      return FALSE;
    }
  return TRUE;
}

static Bool
multitouchInitScreen (CompPlugin * p, CompScreen * s)
{
  MultitouchScreen *ms;

  MULTITOUCH_DISPLAY (s->display);

  ms = malloc (sizeof (MultitouchScreen));
  if (!ms)
    return FALSE;

  s->base.privates[md->screenPrivateIndex].ptr = ms;

  return TRUE;
}

static void
multitouchFiniScreen (CompPlugin * p, CompScreen * s)
{
  MULTITOUCH_SCREEN (s);

  free (ms);
}

static Bool
multitouchInitDisplay (CompPlugin * p, CompDisplay * d)
{
// Generate a Multitouch display
  MultitouchDisplay *md;
//   int i;
  if (!checkPluginABI ("core", CORE_ABIVERSION))
    return FALSE;
  md = malloc (sizeof (MultitouchDisplay));
  if (!md)
    return FALSE;
// Allocate a private index
  md->screenPrivateIndex = allocateScreenPrivateIndex (d);
// Check if its valid
  if (md->screenPrivateIndex < 0)
    {
// Its invalid so free memory and return
      free (md);
      return FALSE;
    }
  md->enabled = FALSE;
  md->eventsThread = 0;
  md->endThread = FALSE;
  pthread_mutex_init (&md->lock, NULL);
// Record the display
  d->base.privates[displayPrivateIndex].ptr = md;

  multitouchSetToggleKeyInitiate (d, multitouchToggle);

  if (!multitouchInitThread (d))
    {
      pthread_mutex_destroy (&md->lock);
      free (md);
    }
  return TRUE;
}

static void
multitouchFiniDisplay (CompPlugin * p, CompDisplay * d)
{
  int status, id;//, retval, i;
  MULTITOUCH_DISPLAY (d);
  union semun
  {
    int val;
    struct semid_ds *buf;
    ushort *array;
  } argument;
  argument.val = 2;
// Free the private index
  freeScreenPrivateIndex (d, md->screenPrivateIndex);
// If thread is still there (the opposite would be seriously worrying),
// lock the mutex, set termination flag, unlock the mutex and wait for
// the thread.
  if (md->eventsThread)
    {
      pthread_mutex_lock (&md->lock);

      md->endThread = TRUE;
// Prepairing the thread, unpause the semaphore in case it's waiting
      id = semget (KEY, 1, 0666);
      if (id < 0)
	{
	  compLogMessage (d, "multitouch", CompLogLevelError,
			  "Error: Cannot find semaphore on FiniDisplay (error code : %d).",
			  id);
	}
      if (semctl (id, 0, SETVAL, argument) < 0)
	{
	  compLogMessage (d, "multitouch", CompLogLevelFatal,
			  "In multitouchFiniDisplay Cannot set semaphore value.");
// 	  return FALSE;
	}
// Now unlock the mutex and wait for child thread to join

      pthread_mutex_unlock (&md->lock);

      status = pthread_join (md->eventsThread, NULL);
      if (status)
	compLogMessage (d, "multitouch", CompLogLevelError,
			"pthread_join () failed (error code : %d).",
			status);
    }
  pthread_mutex_destroy (&md->lock);
// Free the pointer
  free (md);
}

static CompBool
multitouchInitObject (CompPlugin * p, CompObject * o)
{
  static InitPluginObjectProc dispTab[] = {
    (InitPluginObjectProc) 0,	/* InitCore */
    (InitPluginObjectProc) multitouchInitDisplay,
    (InitPluginObjectProc) multitouchInitScreen
  };

  RETURN_DISPATCH (o, dispTab, ARRAY_SIZE (dispTab), TRUE, (p, o));
}

static void
multitouchFiniObject (CompPlugin * p, CompObject * o)
{
  static FiniPluginObjectProc dispTab[] = {
    (FiniPluginObjectProc) 0,
    (FiniPluginObjectProc) multitouchFiniDisplay,
    (FiniPluginObjectProc) multitouchFiniScreen
  };

  DISPATCH (o, dispTab, ARRAY_SIZE (dispTab), (p, o));
}

static Bool
multitouchInit (CompPlugin * p)
{
  displayPrivateIndex = allocateDisplayPrivateIndex ();
  if (displayPrivateIndex < 0)
    return FALSE;

  return TRUE;
}

static void
multitouchFini (CompPlugin * p)
{
  freeDisplayPrivateIndex (displayPrivateIndex);
}

CompPluginVTable multitouchVTable = {
  "multitouch",
  0,
  multitouchInit,
  multitouchFini,
  multitouchInitObject,
  multitouchFiniObject,
  0,
  0
};

CompPluginVTable *
getCompPluginInfo (void)
{
  return &multitouchVTable;
}
