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

#define MAXBLOBS (20)

#include <compiz-core.h>
#include "multitouch_options.h"

#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <math.h>

#include <sys/time.h>
#include <strings.h>
#include <unistd.h>

#include <X11/Xlib.h>
#include <lo/lo.h>

CompDisplay *firstDisplay;
static int displayPrivateIndex;
static int corePrivateIndex;

typedef enum
{
    EventUp = 1,
    EventDown = 2,
    EventMove = 3
}mtEvent;

typedef enum
{
    Disabled = 0,
    Annotate = 1,
    Ripple = 2
}mtEffect;

typedef struct
{
    int id,w;
    float x, y, xmot,ymot, mot_accel,width,height,oldx,oldy;
} mtblob;

typedef struct
{
    unsigned short R,G,B,A;
} mtcolor;

/* This struct is used as closure for timeouts */
typedef struct _DisplayValue
{
    CompDisplay *display;
    float x0;
    float y0;
    float x1;
    float y1;
    mtEvent mtevent;
} DisplayValue;

typedef struct _MultitouchDisplay
{
    int screenPrivateIndex;
    char port[6];
    int fwdport;
    Bool enabled;
    Bool Debug;
    Bool Wm;
    Bool TuioFwd;
    lo_server_thread st;
    mtblob blob[MAXBLOBS];
    mtcolor color;
    CompTimeoutHandle timeoutHandles;
    CompTimeoutHandle clickTimeoutHandle;
} MultitouchDisplay;

typedef struct _MultitouchScreen
{
    mtEffect CurrentEffect;
} MultitouchScreen;

#define GET_MULTITOUCH_DISPLAY(d) \
((MultitouchDisplay *) (d)->base.privates[displayPrivateIndex].ptr)
#define MULTITOUCH_DISPLAY(d) \
MultitouchDisplay *md = GET_MULTITOUCH_DISPLAY (d)
#define GET_MULTITOUCH_SCREEN(s, md) \
((MultitouchScreen *) (s)->base.privates[(md)->screenPrivateIndex].ptr)
#define MULTITOUCH_SCREEN(s) \
MultitouchScreen *ms = GET_MULTITOUCH_SCREEN (s, GET_MULTITOUCH_DISPLAY (s->display))

/* TODO: draw debug info on cairo, animated effect for each blob, pie (radial) menu */

/*
 * get the topmost xlib window id from x,y coordinates
 * name: findWindowAtPoint
 * @param x,y
 * @return wid
 */

static int findWindowAtPoint ( int x, int y)
{
    /*
     * To do this properly, we need to open another client connection to X
     * to prevent accidentall locks and segfaults as this is threaded call
     */
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
                if ( ( tx < x ) && ( ty < y ) && ( ( winattr.width + tx ) > x ) && ( ( winattr.height + ty ) > y ) )
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

/*
 * Send commands trough action system of the compiz to other plugins
 * name: sendInfoToPlugin
 * @param display, argument, nArgument, pluginName, actionName
 * @return bool
 */

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

/* Change notify for bcop
 * TODO: doesn't work at the moment 4 unknown reason */
static void
multitouchDisplayOptionChanged (CompDisplay             *d,
			       CompOption              *opt,
			       MultitouchDisplayOptions num)
{
    MULTITOUCH_DISPLAY (d);
    switch (num) {
    case MultitouchDisplayOptionPort:
    sprintf (md->port,"%d",multitouchGetPort (d));
    if (md->Debug)
	    printf ("notify port:%s\n",md->port);
	break;
    case MultitouchDisplayOptionEnableFwd:
	md->TuioFwd = multitouchGetEnableFwd(d);
    if (md->Debug)
        printf ("notify fwd:%d\n",md->TuioFwd);
	break;
    case MultitouchDisplayOptionFwdport:
    md->fwdport = multitouchGetFwdport(d);
    if (md->Debug)
        printf ("notify portfwd:%d\n",md->fwdport);
	break;
    default:
	break;
    }
}

static Bool
multitouchToggleDebug (CompDisplay *d,
                  CompAction * action,
                  CompActionState state, CompOption * option, int nOption)
{
    MULTITOUCH_DISPLAY(d);
    md->Debug = !md->Debug;
    if (md->Debug)
        printf ("Debug enabled\n");
    else printf ("Debug disabled\n");
    return FALSE;
}

static Bool
multitouchToggleWm (CompDisplay *d,
                  CompAction * action,
                  CompActionState state, CompOption * option, int nOption)
{
    MULTITOUCH_DISPLAY(d);
    md->Wm = !md->Wm;
    if (md->Debug && md->Wm)
        printf ("Gestures enabled\n");
    else
    {
    if (md->Debug)
        printf ("Gestures disabled\n");
    mtblob *blobs = md->blob;
    int i;
    for (i=0;i<MAXBLOBS; i++)
        {
        if ( blobs[i].w )   // cleaning window id from blobs
            blobs[i].w = 0;
        }
    }
    return FALSE;
}

static Bool
multitouchToggleFwd (CompDisplay *d,
                  CompAction * action,
                  CompActionState state, CompOption * option, int nOption)
{
        MULTITOUCH_DISPLAY(d);
        md->TuioFwd = !md->TuioFwd;
        if (md->Debug && md->TuioFwd)
            printf ("Tuio Forwarding enabled\n");
        else if (md->Debug)
            printf ("Tuio Forwarding disabled\n");
    return FALSE;
}

static Bool
multitouchToggleEffects (CompDisplay *d,
                  CompAction * action,
                  CompActionState state, CompOption * option, int nOption)
{
    CompScreen *s;
    s = findScreenAtDisplay (d, currentRoot);
    if (s)
    {
        MULTITOUCH_DISPLAY(d);
        MULTITOUCH_SCREEN (s);
        if (ms->CurrentEffect == Ripple)
            ms->CurrentEffect = Disabled;
        else
            {
            ms->CurrentEffect++;
            if (md->Debug)
                printf ("Current effect num: %d",ms->CurrentEffect);
            }
        if (md->Debug && ms->CurrentEffect)
            printf (" enabled\n");
        else if (md->Debug)
            printf ("Effects disabled\n");
    }
    return FALSE;
}

/* Calls to other plugins */

static Bool
makeripple (void *data)
{
    DisplayValue *dv = (DisplayValue *) data;
    CompScreen *s;
    CompOption arg[6];
    s = findScreenAtDisplay (dv->display, currentRoot);

    if (s)
    {
        int width = s->width;
        int height = s->height;
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
        arg[nArg].value.i = 0.5f;     // 0.25f 4 point,0.5f 4 line

        sendInfoToPlugin (dv->display, arg, nArg, "water", "line");
    }
    free (dv);
    return FALSE;                 // Return False so the timeout gets removed upon first callback
}

static Bool
makeannotate (void *data)
{
    DisplayValue *dv = (DisplayValue *) data;
    CompScreen *s;
    s = findScreenAtDisplay (dv->display, currentRoot);
    //MULTITOUCH_SCREEN(s);
    int width = s->width;
    int height = s->height;
    MULTITOUCH_DISPLAY (dv->display);

    if (s)
    {
        CompOption arg[8];
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

        mtcolor *color = &(md->color);
        arg[nArg].name = "fill_color";
        arg[nArg].type = CompOptionTypeColor;
        arg[nArg].value.c[0]=color->R;
        arg[nArg].value.c[1]=color->G;
        arg[nArg].value.c[2]=color->B;
        arg[nArg].value.c[3]=color->A;
        nArg++;

        arg[nArg].name = "line_width";
        arg[nArg].type = CompOptionTypeFloat;
        arg[nArg].value.f = 1.5f;

        sendInfoToPlugin (dv->display, arg, nArg, "annotate", "draw");
    }
    free (dv);
    return FALSE;
}


static Bool
displaytext (void *data)
{
    DisplayValue *dv = (DisplayValue *) data;
    CompScreen *s;
    CompOption arg[4];
    int nArg = 0;
    s = findScreenAtDisplay (dv->display, currentRoot);

    if (s)
    {
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
    }
    free (dv);
    return FALSE;
}

/*
 * Check if a value is part of set
 * name: isMemberOfSet
 * @param *Set, size, value
 * @return bool
 */

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

static void click_handler(mtEvent event, CompDisplay * d,int BlobID)
{
    MULTITOUCH_DISPLAY (d);
    CompScreen *s;
    s = findScreenAtDisplay (d, currentRoot);
    mtblob *blobs = md->blob;
    int k,multiblobs;
    int wid = 0;
    MULTITOUCH_SCREEN(s);
    switch (event)
    {
    case EventMove:
        if (md->Debug)
            printf("Move handler - blobID: %d\n",blobs[BlobID].id);
        if (ms->CurrentEffect)
        {
        DisplayValue *dv = malloc (sizeof (DisplayValue));
        dv->display = d;
        dv->x0 = blobs[BlobID].x;
        dv->y0 = blobs[BlobID].y;
        dv->x1 = blobs[BlobID].oldx;
        dv->y1 = blobs[BlobID].oldy;
        if (ms->CurrentEffect == Annotate)
            md->timeoutHandles = compAddTimeout (0,makeannotate, dv);
        else md->timeoutHandles = compAddTimeout (0,makeripple, dv);
        }
        if ( blobs[BlobID].w ) // We have window id attached to blob
        {
            for (k=0;k<MAXBLOBS; k++)
            {
                multiblobs = FALSE;
                if ( k != BlobID && blobs[k].w ) // We have multiple blobs per window
                {
                    multiblobs = k;
                    break;
                }
            }
            if (multiblobs && blobs[multiblobs].oldx) // Multiple blobs, do the resizing
            {
                CompWindow *  w = (void *) blobs[BlobID].w;
                int dx = (blobs[BlobID].x - blobs[BlobID].oldx) * s->width;
                int dy = (blobs[BlobID].y - blobs[BlobID].oldy) * s->height;
                resizeWindow(w, dx, dy, w->attrib.width, w->attrib.height, w->attrib.border_width);
            }
            else // Single blob, do just the movement
            {
                int dx = (blobs[BlobID].x - blobs[BlobID].oldx) * s->width;
                int dy = (blobs[BlobID].y - blobs[BlobID].oldy) * s->height;
                if (md->Debug)
                    printf("movewindow id: %d, x%d , y:%d\n", blobs[BlobID].w, dx,dy );
                CompWindow *  w = (void *) blobs[BlobID].w;
                moveWindow(w, dx, dy, TRUE, FALSE);
            }
        }
        break;
    case EventDown:
        if (md->Wm)
            wid = findWindowAtPoint ((int) s->width * blobs[BlobID].x,(int) s->height * blobs[BlobID].y);
        if (md->Debug)
            printf("Down handler - blobID: %d WindowID: %d\n",blobs[BlobID].id, wid);
        if (wid)
        {
            CompWindow * w = (CompWindow *) findWindowAtDisplay (s->display, wid);
            blobs[BlobID].w = (int) w;
            (*w->screen->windowGrabNotify)(w,w->attrib.x + (w->width / 2),w->attrib.y + (w->height / 2),
                                           0 ,CompWindowGrabMoveMask | CompWindowGrabButtonMask);
        }
        break;
    case EventUp:
        if (md->Debug)
            printf("Up handler - blobID: %d\n",blobs[BlobID].id);
        if (blobs[BlobID].w)
        {
            CompWindow * w = (void *) blobs[BlobID].w;
            (*w->screen->windowUngrabNotify)(w);
            syncWindowPosition(w);
        }
        blobs[BlobID].id = 0;
        blobs[BlobID].x = 0;
        blobs[BlobID].y = 0;
        blobs[BlobID].w = 0;
        break;
    default:
        break;
    }
}

/* Callback handlers */

static void loerror(int num, const char *msg, const char *path)
{
    compLogMessage (firstDisplay, "multitouch", CompLogLevelFatal,
                    "liblo server error %d in path %s: %s", num, path, msg);
}

static int tuioFwd_handler(const char *path, const char *types, lo_arg **argv, int argc,
                           void *data, void *display)
{
    //CompScreen *s = (CompScreen *) screen;
    //lo_address t = lo_address_new(NULL, "3330")
    int j;
    for (j=0; j<argc; j++)
    {
        printf("arg %d '%c' ", j, types[j]);
        lo_arg_pp(types[j], argv[j]);
        printf("\n");
    }
    return 0;
}

static int command_handler(const char *path, const char *types, lo_arg **argv, int argc,
                           void *data, void *display)
{
    CompDisplay *d = (CompDisplay *) display;
    CompScreen *s;
    s = findScreenAtDisplay (d, currentRoot);
    MULTITOUCH_DISPLAY (d);
    MULTITOUCH_SCREEN (s);
    int j;
    mtcolor *color = &(md->color);
    if ( !strcmp((char *) argv[0],"color"))
    {
        color->R = argv[1]->i;
        color->G = argv[2]->i;
        color->B = argv[3]->i;
        color->A = argv[4]->i;
    }
    else if ( !strcmp((char *) argv[0],"toggle"))
    {
        if ( !ms->CurrentEffect )
            ms->CurrentEffect = Annotate;
        else if (ms->CurrentEffect == Annotate)
            ms->CurrentEffect = Ripple;
    }
    else
    {
        for (j=0; j<argc; j++)
        {
            printf("arg %d '%c' ", j, types[j]);
            lo_arg_pp(types[j], argv[j]);
            printf("\n");
        }
    }
    return 0;
}

static int tuio2Dobj_handler(const char *path, const char *types, lo_arg **argv, int argc,
                             void *data, void *display)
{
    CompDisplay *d = (CompDisplay *) display;
    MULTITOUCH_DISPLAY (d);
    return md->TuioFwd;
    int j;
    for (j=0; j<argc; j++)
    {
        printf("arg %d '%c' ", j, types[j]);
        lo_arg_pp(types[j], argv[j]);
        printf("\n");
    }
    return md->TuioFwd;
}

static int tuio2Dcur_handler(const char *path, const char *types, lo_arg **argv, int argc,
                             void *data, void *display)
{
    CompDisplay *d = (CompDisplay *) display;
    CompScreen *s;
    s = findScreenAtDisplay (d, currentRoot);

    MULTITOUCH_DISPLAY (d);
    //MULTITOUCH_SCREEN (s);
    mtblob *blobs = md->blob;
    int j,alive[MAXBLOBS];
    int found = 0;
    if ( !strcmp((char *) argv[0],"set") && argv[1]->i) //bypasing first blob (id 0) as it's giving me headache
    {
        for (j = 0; j < MAXBLOBS; j++)
        {
            /* we are already in the list, so just update x,y (MOVE EVENT) */
            if (blobs[j].id == argv[1]->i)
            {
                blobs[j].oldx = blobs[j].x; // store old X in blobs[] also
                blobs[j].oldy = blobs[j].y;
                blobs[j].x = argv[2]->f;
                blobs[j].y = argv[3]->f;
                click_handler(EventMove, s->display,j); 
                found = 1;
                break;
            }
        }
        /* do we have new blob? (TOUCH DOWN EVENT) */
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
            if (slot != -1)
            {
                blobs[j].id = argv[1]->i;
                blobs[j].x = argv[2]->f;
                blobs[j].y = argv[3]->f;
                blobs[j].xmot = argv[4]->f;
                blobs[j].ymot = argv[5]->f;
                blobs[j].mot_accel = argv[6]->f;
                click_handler(EventDown, s->display, j);
            }
        }
    }
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
                /* check if we have blobs that alive packet doesn't have (TOUCH UP EVENT) */
                if (!(isMemberOfSet (alive, MAXBLOBS, blobs[j].id)))
                {
                    click_handler(EventUp, s->display, j);
                    break;
                }
            }
        }
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
    return md->TuioFwd;
}

static Bool
multitouchToggleMultitouch (CompDisplay *d,
                  CompAction * action,
                  CompActionState state, CompOption * option, int nOption)
{
        MULTITOUCH_DISPLAY(d);
        md->enabled = !md->enabled;
        if (md->Debug && md->enabled)
        {
            CompScreen *s;
            int j;
            s = findScreenAtDisplay (d, currentRoot);
            MULTITOUCH_SCREEN(s);
            int unsigned short *fillcolor;
            fillcolor = multitouchGetFillColor(s);
            sprintf (md->port,"%d",multitouchGetPort (d));
            ms->CurrentEffect = multitouchGetEffect(d);
            md->TuioFwd = multitouchGetEnableFwd(d);
            md->fwdport = multitouchGetFwdport(d);
            md->color.R = fillcolor[0];
            md->color.G = fillcolor[1];
            md->color.B = fillcolor[2];
            md->color.A = fillcolor[3];
            for (j=0;j<MAXBLOBS;j++)
            {
                md->blob[j].id = 0;
                md->blob[j].w = 0;
                md->blob[j].oldx = 0;
            }
            md->st = lo_server_thread_new(md->port, loerror);
            lo_server_thread_add_method(md->st, "/tuio/2Dobj", NULL, tuio2Dobj_handler, d);
            lo_server_thread_add_method(md->st, "/tuio/2Dcur", NULL, tuio2Dcur_handler, d);
            lo_server_thread_add_method(md->st, "/command", NULL, command_handler, d);
            lo_server_thread_add_method(md->st, NULL, NULL, tuioFwd_handler, d);
            lo_server_thread_start(md->st);
            if (md->Debug)
                printf ("Multitouch enabled Port: %s Forwarding: %d Port-fwd: %d color RGBA: %d %d %d %d\n",md->port, (int) md->TuioFwd, (int) md->fwdport,fillcolor[0],fillcolor[1],fillcolor[2],fillcolor[3]);
        }
        else
        {
        if (md->Debug)
            printf ("Multitouch disabled\n");
            lo_server_thread_free(md->st);
        }
    return FALSE;
}

/* Plugin Initialization */

static Bool
multitouchInitCore (CompPlugin * p, CompCore * c)
{
    if (!checkPluginABI ("core", CORE_ABIVERSION))
        return FALSE;
    displayPrivateIndex = allocateDisplayPrivateIndex ();
    if (displayPrivateIndex < 0)
        return FALSE;
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
    /* Generate a Multitouch display */
    MultitouchDisplay *md;
    if (!checkPluginABI ("core", CORE_ABIVERSION))
        return FALSE;
    md = malloc (sizeof (MultitouchDisplay));
    if (!md)
        return FALSE;
    /* Allocate a private index */
    md->screenPrivateIndex = allocateScreenPrivateIndex (d);
    /* Check if its valid */
    if (md->screenPrivateIndex < 0)
    {
        /* Its invalid so free memory and return */
        free (md);
        return FALSE;
    }
    /* Record the display */
    d->base.privates[displayPrivateIndex].ptr = md;
    //multitouchSetPortNotify (d, multitouchDisplayOptionChanged);
    //multitouchSetEnableFwdNotify (d, multitouchDisplayOptionChanged);
    //multitouchSetFwdportNotify (d, multitouchDisplayOptionChanged);
    md->TuioFwd = multitouchGetEnableFwd(d);
    md->enabled = FALSE;
    md->Debug = TRUE;
    md->Wm = FALSE;
    //sprintf (port,"%d",multitouchGetPort (d));
    //md->TuioFwd = multitouchGetEnableFwd(d);
    //md->fwdport = multitouchGetFwdport(d);
    multitouchSetToggleMultitouchInitiate (d, multitouchToggleMultitouch);
    multitouchSetToggleDebugInitiate (d, multitouchToggleDebug);
    multitouchSetToggleFwdInitiate (d, multitouchToggleFwd);
    multitouchSetToggleWmInitiate (d, multitouchToggleWm);
    multitouchSetToggleEffectsInitiate (d, multitouchToggleEffects);
    return TRUE;
}

static void
multitouchFiniDisplay (CompPlugin * p, CompDisplay * d)
{
    MULTITOUCH_DISPLAY (d);
    /* Free the private index */
    freeScreenPrivateIndex (d, md->screenPrivateIndex);
    /* Free the pointer */
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
