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
    ActionAnnotate = 1,
    ActionRipple = 2
}TouchAction;

typedef struct
{
    int id,w;
    float x, y, xmot,ymot, mot_accel,width,height,oldx,oldy;
} mtblob;

typedef struct
{
    unsigned short R,G,B,A;
} mtcolor;

typedef struct _MultitouchDisplay
{
    int screenPrivateIndex;
    mtblob blob[MAXBLOBS];
    mtcolor color;
    CompTimeoutHandle timeoutHandles;
    TouchAction CurrentEffect;
} MultitouchDisplay;

typedef struct _MultitouchScreen
{
    int windowPrivateIndex;
    lo_server_thread st;
    Bool movewins;
    Bool TuioFwdEnabled;
    float line_width;
} MultitouchScreen;

/* This struct is used as closure for timeouts */
typedef struct _DisplayValue
{
    CompDisplay *display;
    float x0;
    float y0;
    float x1;
    float y1;
    //mtcolor *color;
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
 * draw debug info on cairo, animated effect for each blob, pie (radial) menu
 */

/*
 * get the topmost xlib window id from x,y coordinates
 * name: point2wid
 * @param pointx,pointy
 * @return wid
 */

static int point2wid( int pointx, int pointy)
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

static Bool
multitouchToggle (CompScreen *s,
                  CompAction * action,
                  CompActionState state, CompOption * option, int nOption)
{
    MULTITOUCH_SCREEN(s);
    ms->TuioFwdEnabled = !ms->TuioFwdEnabled;
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
        printf("Adresa: %p %x Podatci za crtanje: %d %d %d %d Podatci iz memorije: %d %d %d %d\n",color,(unsigned int) color,
            arg[nArg].value.c[0],arg[nArg].value.c[1],arg[nArg].value.c[2],arg[nArg].value.c[3],color->R,color->G,color->B,color->A);
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

/* Callback handlers */

static void loerror(int num, const char *msg, const char *path)
{
    compLogMessage (firstDisplay, "multitouch", CompLogLevelFatal,
                    "liblo server error %d in path %s: %s", num, path, msg);
}
static int tuioFwd_handler(const char *path, const char *types, lo_arg **argv, int argc,
                           void *data, CompScreen *s)
{
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
                           void *data, CompScreen *s)
{
    MULTITOUCH_DISPLAY (s->display);
    int j;
    mtcolor *color = &(md->color);
    if ( !strcmp((char *) argv[0],"color"))
    {
        printf("Adresa: %p Dobijeno: %d %d %d %d\n", color, argv[1]->i,argv[2]->i,argv[3]->i,argv[4]->i);
        color->R = argv[1]->i;
        color->G = argv[2]->i;
        color->B = argv[3]->i;
        color->A = argv[4]->i;
        printf("Adresa: %p Stanje u memoriji: %d %d %d %d\n", color,color->R,color->G,color->B,color->A);
    }
    else if ( !strcmp((char *) argv[0],"toggle"))
    {
        if ( md->CurrentEffect == ActionAnnotate )
            md->CurrentEffect = ActionRipple;
        else md->CurrentEffect = ActionAnnotate;
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
                             void *data, CompScreen *s)
{
    MULTITOUCH_SCREEN (s);
    int j;
    for (j=0; j<argc; j++)
    {
        printf("arg %d '%c' ", j, types[j]);
        lo_arg_pp(types[j], argv[j]);
        printf("\n");
    }
    return ms->TuioFwdEnabled;
}

static int tuio2Dcur_handler(const char *path, const char *types, lo_arg **argv, int argc,
                             void *data, CompScreen *s)
{
    MULTITOUCH_DISPLAY (s->display);
    MULTITOUCH_SCREEN (s);
    mtblob *blobs = md->blob;
    int j,k,multiblobs,alive[MAXBLOBS];
    int found = 0;
    if ( !strcmp((char *) argv[0],"set") && argv[1]->i) //bypasing first blob (id 0) as it's giving me headache
    {
        for (j = 0; j < MAXBLOBS; j++)
        {
            /* we are already in the list, so just update x,y (move handler) */
            if (blobs[j].id == argv[1]->i)
            {
                DisplayValue *dv = malloc (sizeof (DisplayValue));
                if (!dv)
                    return FALSE;
                //printf("move handler - blobID (i):%d\n",blobs[j].id);
                dv->display = s->display;
                dv->x0 = blobs[j].x; // put old X coordinates
                dv->y0 = blobs[j].y;
                dv->x1 = argv[2]->f; // new X coordinates  makeripple
                dv->y1 = argv[3]->f;
                //dv->color = &md->color;
                md->timeoutHandles = compAddTimeout (0,makeannotate, dv);
                blobs[j].x = argv[2]->f;
                blobs[j].y = argv[3]->f;
                blobs[j].oldx = dv->x0; // store old X in blobs[] also
                blobs[j].oldy = dv->y0;
                found = 1;
                if ( blobs[j].w ) // We have window id attached to blob
                {
                    for (k=0;k<MAXBLOBS; k++)
                    {
                        multiblobs = FALSE;
                        if ( k != j && blobs[k].w ) // We have multiple blobs per window
                        {
                            multiblobs = k;
                            break;
                        }
                    }
                    if (multiblobs && blobs[multiblobs].oldx)
                    {
                        CompWindow *  w = (void *) blobs[j].w;
                        int dx = (dv->x1 - dv->x0) * s->width;
                        int dy = (dv->y1 - dv->y0) * s->height;
                        resizeWindow(w, dx, dy, w->attrib.width, w->attrib.height, w->attrib.border_width);
                    }
                    else // Single blob, do just the movement
                    {
                        int dx = (dv->x1 - dv->x0) * s->width;
                        int dy = (dv->y1 - dv->y0) * s->height;
                        CompWindow *  w = (void *) blobs[j].w;
                        moveWindow(w, dx, dy, TRUE, FALSE);
                        //printf("movewindow id: %d, x%d , y:%d\n", blobs[j].wid, dx,dy );
                    }
                }
                break;
            } // if
        } // for
        /* do we have new blob? (touch down handler) */
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
                // enable/disable under to have windows movement
                //int wid = point2wid((int) s->width * argv[2]->f,(int) s->height * argv[3]->f);
                int wid = 0;
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
                    (*w->screen->windowGrabNotify)(w,w->attrib.x + (w->width / 2),w->attrib.y + (w->height / 2),
                                                   0 ,CompWindowGrabMoveMask | CompWindowGrabButtonMask);
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
                /* check if we have blobs that alive packet doesn't have (touch up handler) */
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
    return ms->TuioFwdEnabled;
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
    char port[6];
    sprintf (port,"%d",multitouchGetPort (s));
    ms = malloc (sizeof (MultitouchScreen));
    if (!ms)
        return FALSE;
    s->base.privates[md->screenPrivateIndex].ptr = ms;
    //multitouchSetToggleTuioInitiate (s, multitouchToggle);
    ms->TuioFwdEnabled = FALSE;
    ms->st = lo_server_thread_new(port, loerror);
    lo_server_thread_add_method(ms->st, "/tuio/2Dobj", NULL, tuio2Dobj_handler,(void *) s);
    lo_server_thread_add_method(ms->st, "/tuio/2Dcur", NULL, tuio2Dcur_handler,(void *) s);
    lo_server_thread_add_method(ms->st, "/command", NULL, command_handler,(void *) s);
    lo_server_thread_add_method(ms->st, NULL, NULL, tuioFwd_handler,(void *) s);
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
    /* Generate a Multitouch display */
    MultitouchDisplay *md;
    int j;
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
    //md->color.R = 0x0;
    //md->color.G = 0x0;
    //md->color.B = 0x0;
    //md->color.A = 0xffff;

    for (j=0;j<MAXBLOBS;j++)
    {
        md->blob[j].id = 0;
        md->blob[j].w = 0;
        md->blob[j].oldx = 0;
    }
    /* Record the display */
    d->base.privates[displayPrivateIndex].ptr = md;
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
