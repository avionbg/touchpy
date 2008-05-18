/*
 * Multitouch input plugin
 * multitouch.c
 *
 * Author : Goran Medakovic
 * Email : avion.bg@gmail.com
 *
 * Copyright (c) 2008 Goran Medakovic <avion.bg@gmail.com>
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License
 * as published by the Free Software Foundation; either version 3
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

#define KEY (1972)
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

#include <sys/time.h>
#include <strings.h>
#include <unistd.h>

#include <X11/Xlib.h>
#include <lo/lo.h>

CompDisplay *firstDisplay;
static int displayPrivateIndex;
static int corePrivateIndex;

typedef struct
{
    int id,w;
    float x, y, xmot,ymot, mot_accel,width,height;
} command;

typedef struct
{
    command comm;
    int alive[MAXBLOBS];
} tuio;

typedef struct _MultitouchDisplay
{
    int screenPrivateIndex;

    pthread_t eventsThread;
    pthread_mutex_t lock;
    Bool endThread;
    Bool tuioenabled;

    command blob[MAXBLOBS];
    CompTimeoutHandle timeoutHandles;
} MultitouchDisplay;

typedef struct _MultitouchScreen
{
    int windowPrivateIndex;
    Bool title;
    lo_server_thread st;
} MultitouchScreen;


// This struct is used as closure for timeouts
typedef struct _DisplayValue
{
    CompDisplay *display;
    float x0;
    float y0;
    float x1;
    float y1;
} DisplayValue;

#define GET_MULTITOUCH_DISPLAY(d) \
((MultitouchDisplay *) (d)->base.privates[displayPrivateIndex].ptr)
#define MULTITOUCH_DISPLAY(d) \
MultitouchDisplay *md = GET_MULTITOUCH_DISPLAY (d)
#define GET_MULTITOUCH_SCREEN(s, md) \
((MultitouchScreen *) (s)->base.privates[(md)->screenPrivateIndex].ptr)
#define MULTITOUCH_SCREEN(s) \
MultitouchScreen *ms = GET_MULTITOUCH_SCREEN (s, GET_MULTITOUCH_DISPLAY (s->display))

/* TODO: Add toggling the effects trough key/gestures
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

/*
 * get the topmost xlib window id from x,y coordinates
 * name: point2wid
 * @param pointx,pointy
 * @return wid
 */

static int point2wid( int pointx, int pointy)
{
// To do this properly, we need to open another client connection to xlib
// to prevent accidentall locks and segfaults as this is threaded call
    char *display_name;
    Display *d;
    if ( (display_name = getenv("DISPLAY")) == (void *)NULL)
    {
        compLogMessage (firstDisplay, "multitouch", CompLogLevelError,
                        "Error: DISPLAY environment variable not set!");
        return 0;
    }
    if ((d = XOpenDisplay(display_name)) == NULL)
    {
        compLogMessage (firstDisplay, "multitouch", CompLogLevelError,
                        "Error: Can't open display: %s!", display_name);
        return 0;
    }
    unsigned int j, nwins;
    int tx, ty,wid;
    Window root_return, parent_return, *wins;
    XWindowAttributes winattr;
    Window root = currentRoot;
    XQueryTree(d , root, &root_return, &parent_return, &wins, &nwins);
    for (j = 1; j < nwins; j++)
    {
        XGetWindowAttributes(d , wins[j], &winattr);
        if (!winattr.override_redirect && winattr.map_state == IsViewable)
        {
            if (wins[j]==winattr.root)
            {
                tx = 0;
                ty = 0;
            }
            else
            {
                Window child_return;
                XTranslateCoordinates(d, root, winattr.root,
                                      winattr.x, winattr.y, &tx, &ty, &child_return);
                if ( ( tx < pointx ) && ( ty < pointy ) && ( ( winattr.width + tx ) > pointx ) && ( ( winattr.height + ty ) > pointy ) )
                    wid = wins[j] ;
            }
        }
    }
    XFree(wins);
    XCloseDisplay(d);
    if (wid)
    {
        CompWindow *cwin;
        cwin = findWindowAtDisplay (firstDisplay, wid);
        // In case this is dock (panel) window return 0
        if (cwin->type & CompWindowTypeDockMask ) return 0;
    }
    return wid;
}

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

    md->tuioenabled = !md->tuioenabled;

    return FALSE;
}

// Calls to other plugins
static Bool
makeripple (void *data)
{
    DisplayValue *dv = (DisplayValue *) data;
    CompScreen *s;
    s = findScreenAtDisplay (dv->display, currentRoot);
    int width = s->width;
    int height = s->height;

    CompOption arg[6];
    int nArg = 0;

    arg[nArg].name = "root";
    arg[nArg].type = CompOptionTypeInt;
    arg[nArg].value.i = s->root;
    nArg++;

    arg[nArg].name = "x0";
    arg[nArg].type = CompOptionTypeInt;
    arg[nArg].value.i = (dv->x0 * width);
    nArg++;

    arg[nArg].name = "y0";
    arg[nArg].type = CompOptionTypeInt;
    arg[nArg].value.i = (dv->y0 * height);
    nArg++;

    arg[nArg].name = "x1";
    arg[nArg].type = CompOptionTypeInt;
    arg[nArg].value.i = (dv->x1 * width);
    nArg++;

    arg[nArg].name = "y1";
    arg[nArg].type = CompOptionTypeInt;
    arg[nArg].value.i = (dv->y1 * height);
    nArg++;

    arg[nArg].name = "amplitude";
    arg[nArg].type = CompOptionTypeFloat;
    arg[nArg].value.i = 0.5f;     // 0.25f za point,0.5f za line

    sendInfoToPlugin (dv->display, arg, nArg, "water", "line");
    free (dv);
    return FALSE;                 // Return False so the timeout gets removed upon first callback
}

static Bool
makeannotate (void *data)
{

    DisplayValue *dv = (DisplayValue *) data;
    CompScreen *s;
    s = findScreenAtDisplay (dv->display, currentRoot);
    int width = s->width;
    int height = s->height;

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
        arg[nArg].value.f = (float) (dv->x0 * width);
        nArg++;

        arg[nArg].name = "y1";
        arg[nArg].type = CompOptionTypeFloat;
        arg[nArg].value.f = (float) (dv->y0 * height);
        nArg++;

        arg[nArg].name = "x2";
        arg[nArg].type = CompOptionTypeFloat;
        arg[nArg].value.f = (float) (dv->x1 * width);
        nArg++;

        arg[nArg].name = "y2";
        arg[nArg].type = CompOptionTypeFloat;
        arg[nArg].value.f = (float) (dv->y1 * height);
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
    return FALSE;                 // Return False so the timeout gets removed upon first callback
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
    return FALSE;                 // Return False so the timeout gets removed upon first callback
}

// Check if a value is part of set
static int
isMemberOfSet (int *Set, int size, int value)
{
    int i;
    for (i = 0; i < size; ++i)
    {
        if (value == Set[i])
        {
            return 1;
        }
    }
    return 0;
}

static int tuio_handler(const char *path, const char *types, lo_arg **argv, int argc,
                        void *data, CompScreen *s)
{
    MULTITOUCH_DISPLAY (s->display);

    command *blobs = md->blob;
    int j,alive[MAXBLOBS];
    int found = 0;
    if ( !strcmp((char *) argv[0],"set") && argv[1]->i) //bypasing first blob (id 0) as it's giving me headache
    {
        for (j = 0; j < MAXBLOBS; j++)
        {
// we are already in the list, so just update x,y (move handler)
            if (blobs[j].id == argv[1]->i)
            {
                DisplayValue *dv = malloc (sizeof (DisplayValue));
                if (!dv)
                    return FALSE;
                //printf("move handler - blobID (i):%d\n",blobs[j].id);
                dv->display = s->display;
                dv->x0 = blobs[j].x; // put old X coordinates
                dv->y0 = blobs[j].y;
                dv->x1 = argv[2]->f; // new X coordinates makeannotate
                dv->y1 = argv[3]->f;
                md->timeoutHandles = compAddTimeout (0, makeripple, dv);
                blobs[j].x = argv[2]->f;
                blobs[j].y = argv[3]->f;
                found = 1;
                if ( blobs[j].w )
                {
                    int dx =  (dv->x1 - dv->x0) * s->width;
                    int dy =  (dv->y1 - dv->y0) * s->height;
                    CompWindow *  w = (void *) blobs[j].w;
                    moveWindow(w, dx, dy, TRUE, FALSE);
                    //printf("movewindow id: %d, x%d , y:%d\n", blobs[j].wid, dx,dy );
                }
                break;
            } // if
        } // for
// do we have new blob? (touch down handler)
        if (!found)
        {
            int slot = -1;
            for (j=0; j<MAXBLOBS;j++)
            {
                if (!blobs[j].id)
                {
                    slot = j;
                    break;
                }
            }
            //printf("down handler: slot: %d blobID: %d\n",slot,argv[1]->i);
            if (slot != -1)
            {
                int wid = point2wid((int) s->width * argv[2]->f,(int) s->height * argv[3]->f);
                //printf ("wid:%d \n", wid);
                blobs[j].id = argv[1]->i;
                blobs[j].x = argv[2]->f;
                blobs[j].y = argv[3]->f;
                blobs[j].xmot = argv[4]->f;
                blobs[j].ymot = argv[5]->f;
                blobs[j].mot_accel = argv[6]->f;
                if (wid)
                {
                    CompWindow * w = (CompWindow *) findWindowAtDisplay (s->display, wid);
                    blobs[j].w = (int) w;
                    //(*w->screen->windowGrabNotify)(w,(int) blobs[j].x * s->width ,(int) blobs[j].y * s->height,w->state ,CompWindowGrabButtonMask);
                    (*w->screen->windowGrabNotify)(w,w->attrib.x + (w->width / 2),w->attrib.y + (w->height / 2),0 ,CompWindowGrabMoveMask | CompWindowGrabButtonMask);
                }
            } // if
        } // if
    } // if
    else if ( !strcmp((char *) argv[0],"alive"))
    {
        for (j=1;j<argc;j++)
        {
            alive[j] = argv[j]->i;
        }
        for (j = 0; j < MAXBLOBS; j++)
        {
            if (blobs[j].id)
            {
// check if we have blobs that alive packet doesn't have (touch up handler)
                if (!(isMemberOfSet (alive, MAXBLOBS, blobs[j].id)))
                {
                    if (blobs[j].w)
                        {
                        CompWindow * w = (void *) blobs[j].w;
                        (*w->screen->windowUngrabNotify)(w);
                        syncWindowPosition(w);
                        }
                    //printf("up handler - blobID: %d\n",blobs[j].id);
                    blobs[j].id = 0;
                    blobs[j].x = 0;
                    blobs[j].y = 0;
                    blobs[j].w = 0;
                    break;
                }
            } // if
        } // for
    }
    else
    {
        return 0;
        for (j=0; j<argc; j++)   //not handled messagess
        {
            printf("arg %d '%c' ", j, types[j]);
            lo_arg_pp(types[j], argv[j]);
            printf("\n");
        }
    }
    return 0;
}

static void error(int num, const char *msg, const char *path)
{
    compLogMessage (firstDisplay, "multitouch", CompLogLevelFatal,
                    "liblo server error %d in path %s: %s", num, path, msg);
}

// Thread which is used to read commands sent over
// shared memory and trigger the main thread trough timeouts
static Bool
multitouchThread (void *display)
{
    CompDisplay *d = (CompDisplay *) display;
    MULTITOUCH_DISPLAY (d);
    int shmid, semid, retval, i;
    tuio *tcomm;
    command *tuio_cmd;
    command *blobs = md->blob;
//  command blob[MAXBLOBS] = { (int) NULL, (int) NULL, (int) NULL };
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
    }                           //if
    tcomm = shmat (shmid, NULL, 0);
    if (tcomm == (void *) -1)
    {
        compLogMessage (d, "multitouch", CompLogLevelFatal,
                        "Error attaching memory(error code: %d).", tcomm);
        return FALSE;
    }                           //if

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
    while (1)
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
        tuio_cmd = &(tcomm->comm);
        int found = 0;
        int slot = 0;
        for (i = 0; i < MAXBLOBS; i++)
        {
            if (blobs[i].id)
            {
// check if we have blobs that alive packet doesn't have (deleted blob)
                if (!(isMemberOfSet (tcomm->alive, MAXBLOBS, blobs[i].id)))
                {
                    blobs[i].id = 0;
                    blobs[i].x = 0;
                    blobs[i].y = 0;
// no slot allocated (we are 0th member)
                    if (!slot)
                        slot = i + 1;
                    break;
                }
            } // if
// to avoid 0 triggering again this condition, add 1 to i
            else if (!slot)
                slot = i + 1;
        } // for

        for (i = 0; i < MAXBLOBS; i++)
        {
// we are already in the list, su just update x,y
            if (blobs[i].id == tuio_cmd->id)
            {
                DisplayValue *dv = malloc (sizeof (DisplayValue));
                if (!dv)
                    return FALSE;
                dv->display = d;
                dv->x0 = blobs[i].x;
                dv->y0 = blobs[i].y;
                dv->x1 = tuio_cmd->x;
                dv->y1 = tuio_cmd->y;
                md->timeoutHandles = compAddTimeout (0, makeannotate, dv);
                blobs[i].x = tuio_cmd->x;
                blobs[i].y = tuio_cmd->y;
                found = 1;
                break;
            }
            else
                found = 0;
        } // for

// do we have free slot and new blob?
        if (slot && !found)
        {
            memcpy (&(blobs[(slot - 1)]), tuio_cmd, sizeof (*tuio_cmd));   // copy the blob into slot position
        }


        operations[0].sem_num = 0;
        operations[0].sem_op = -1;
        operations[0].sem_flg = 0;
        retval = semop (semid, operations, 1);
    }

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
    }                           //if
    if (shmctl (shmid, IPC_RMID, 0) == -1)
    {
        compLogMessage (d, "multitouch", CompLogLevelFatal,
                        "Error: shmctl(IPC_RMID) failed.");
        return FALSE;
    }                           //if
    return 0;
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
                        (CompDisplay *) d);
    pthread_attr_destroy (&attr);
    if (status)
    {
        compLogMessage (d, "multitouch", CompLogLevelFatal,
                        "Could not start thread (error code: %d).", status);
        return FALSE;
    }
    return TRUE;
}

// Plugin Core Initialization
static Bool
multitouchInitCore (CompPlugin * p, CompCore * c)
{
    if (!checkPluginABI ("core", CORE_ABIVERSION))
        return FALSE;

    displayPrivateIndex = allocateDisplayPrivateIndex ();
    if (displayPrivateIndex < 0)
    {
        return FALSE;
    }

    firstDisplay = c->displays; // Use initCore() to find the first display (c->displays)

    return TRUE;
}

static void
multitouchFiniCore (CompPlugin * p, CompCore * c)
{
    freeDisplayPrivateIndex (displayPrivateIndex);
}

static Bool
multitouchInitScreen (CompPlugin * p, CompScreen * s)
{
    MultitouchScreen *ms;
    MULTITOUCH_DISPLAY (s->display);
    char port[6];
    sprintf (port,"%d",multitouchGetPort (s));
    ms = malloc (sizeof (MultitouchScreen));
    if (!ms)
        return FALSE;
    s->base.privates[md->screenPrivateIndex].ptr = ms;
    ms->st = lo_server_thread_new(port, error);
    lo_server_thread_add_method(ms->st, "/tuio/2Dcur", NULL, tuio_handler,(void *) s);
    lo_server_thread_start(ms->st);

    return TRUE;
}

static void
multitouchFiniScreen (CompPlugin * p, CompScreen * s)
{
    MULTITOUCH_SCREEN (s);
    lo_server_thread_free(ms->st);

    free (ms);
}

static Bool
multitouchInitDisplay (CompPlugin * p, CompDisplay * d)
{
// Generate a Multitouch display
    MultitouchDisplay *md;
    int j;
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

    for (j=0;j<MAXBLOBS;j++)
    {
        md->blob[j].id = 0;
        md->blob[j].w = 0;
    }

    md->tuioenabled = FALSE;
    md->eventsThread = 0;
    md->endThread = FALSE;
    pthread_mutex_init (&md->lock, NULL);
// Record the display
    d->base.privates[displayPrivateIndex].ptr = md;

//     multitouchSetToggleTuioInitiate (d, multitouchToggle);

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
    int status, id;               //, retval, i;
    MULTITOUCH_DISPLAY (d);
    union semun
    {
        int val;
        struct semid_ds *buf;
        ushort *array;
    } argument;
    argument.val = 2;
// If thread is still there (the opposite would be seriously worrying),
// lock the mutex, set termination flag, unlock the mutex and wait for
// the thread.
    if (md->eventsThread)
    {
        pthread_mutex_lock (&md->lock);

        md->endThread = TRUE;
        if (!md->tuioenabled)
        {
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
//        return FALSE;
            }
// Now unlock the mutex and wait for child thread to join
        }
        pthread_mutex_unlock (&md->lock);
        status = pthread_join (md->eventsThread, NULL);
        if (status)
            compLogMessage (d, "multitouch", CompLogLevelError,
                            "pthread_join () failed (error code : %d).",
                            status);
    }
    pthread_mutex_destroy (&md->lock);
// Free the private index
    freeScreenPrivateIndex (d, md->screenPrivateIndex);
// Free the pointer
    free (md);
}

static CompBool
multitouchInitObject (CompPlugin * p, CompObject * o)
{
    static InitPluginObjectProc dispTab[] =
    {
        (InitPluginObjectProc) multitouchInitCore,   /* InitCore */
        (InitPluginObjectProc) multitouchInitDisplay,
        (InitPluginObjectProc) multitouchInitScreen
    };

    RETURN_DISPATCH (o, dispTab, ARRAY_SIZE (dispTab), TRUE, (p, o));
}

static void
multitouchFiniObject (CompPlugin * p, CompObject * o)
{
    static FiniPluginObjectProc dispTab[] =
    {
        (FiniPluginObjectProc) multitouchFiniCore,
        (FiniPluginObjectProc) multitouchFiniDisplay,
        (FiniPluginObjectProc) multitouchFiniScreen
    };

    DISPATCH (o, dispTab, ARRAY_SIZE (dispTab), (p, o));
}

static Bool
multitouchInit (CompPlugin * p)
{
    corePrivateIndex = allocateCorePrivateIndex ();
    if (corePrivateIndex < 0)
        return FALSE;
    return TRUE;
}

static void
multitouchFini (CompPlugin * p)
{
    freeCorePrivateIndex (corePrivateIndex);
}

CompPluginVTable multitouchVTable =
{
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