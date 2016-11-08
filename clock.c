/*
clock.c Version 1.0.0. Analog clock
Copyright (C) 2016   aquila62 at github.com

This program is free software; you can redistribute it and/or
modify it under the terms of the GNU General Public License
as published by the Free Software Foundation; either version 2
of the License, or (at your option) any later version.

This program is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
GNU General Public License for more details.

You should have received a copy of the GNU General Public License
along with this program; if not, write to:

	Free Software Foundation, Inc.
	59 Temple Place - Suite 330
	Boston, MA  02111-1307, USA.
*/

/* Press 'q' to quit */

/* to define the escape key */
#define XK_MISCELLANY 1
#define XK_LATIN1 1

#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <time.h>
#include <sys/times.h>
#include <sys/types.h>
#include <X11/Xlib.h>
#include <X11/keysymdef.h>
#include <assert.h>
#include <math.h>

#ifndef M_PI
#define M_PI (3.14159265358979323846)
#endif

#define TWOPI (M_PI + M_PI)

#define HALFPI (0.5 * M_PI)

#define SEC (M_PI / 30.0)

/* structure for X Windows meta-data */
typedef struct xxstruct {
   int whiteColor,blackColor;
   int rootwh;      /* root window height */
   int rootww;      /* root window width  */
   int dpyhght;     /* display window height */
   int dpywdth;     /* display window width  */
   int repeat;
   int runflg;      /* 1=run 0=stop */
   unsigned long red;
   unsigned long blue;
   Display *dpy;
   Window w;
   GC gc;
   Font fontid;
   Screen *dfltscr;
   Colormap cmap;
   XColor scrdef,exctdef;
   unsigned int x1;                /* source x coordinate */
   unsigned int x2;                /* target x coordinate */
   unsigned int y1;                /* source y coordinate */
   unsigned int y2;                /* target y coordinate */
   int hh;                         /* current hour   */
   int mm;                         /* current minute */
   int ss;                         /* current second */
   double tick;                    /* one second in radians */
   char weekday[16*7];             /* table of days of the week */
   } xxfmt;

/* X Windows code is based on: */
/* http://tronche.com/gui/x/xlib-tutorial/2nd-program-anatomy.html */
/* or */
/* http://tronche.com/gui/x  */

/* wait until key pressed */
void getkey(xxfmt *xx)
   {
   /* after each pause wait for command or exposure */

   XSelectInput(xx->dpy, xx->w,
      KeyPressMask|ExposureMask);

   while(1)
      {
      int symbol;
      XEvent e;
      XKeyEvent *k;
      XNextEvent(xx->dpy, &e);
      if (e.type == KeyPress)
         {
         k = (XKeyEvent *) &e;
         symbol = XLookupKeysym(k,0);
         xx->repeat = 0;
         if (symbol == XK_Escape || symbol == 'q')
            {
            xx->repeat = 0;
	    xx->runflg = 0;
            break;
            } /* if quit */
         else if (symbol == 'r' || symbol == 'n')
            {
            xx->repeat = 1;
            break;
            } /* if next */
         } /* if keypress event */
      else if (e.type == Expose)
         {
	 XClearWindow(xx->dpy,xx->w);
         continue;
         } /* if expose event */
      } /* wait for window shutdown */
   } /* getkey */

/* test keyboard for input character */
void ifkey(xxfmt *xx)
   {
   int msk;
   int symbol;
   int XCheckMaskEvent();
   XEvent e;
   XKeyEvent *k;

   msk = KeyPressMask|ExposureMask;

   XSelectInput(xx->dpy, xx->w, msk);

   while (XCheckMaskEvent(xx->dpy, msk, &e))
      {
      if (e.type == KeyPress)
         {
         k = (XKeyEvent *) &e;
         symbol = XLookupKeysym(k,0);
         if (symbol == XK_Escape
	    || symbol == XK_q)
            {
            xx->runflg = 0;
            } /* if quit */
         } /* if keypress event */
      else if (e.type == Expose)
         {
	 XClearWindow(xx->dpy,xx->w);
         } /* if expose event */
      } /* if event received */
   } /* ifkey */

/* initialize X Windows */
void initx(xxfmt *xx)
   {
   int rslt;
   char title[64];

   xx->dpy = XOpenDisplay(NULL);

   if (xx->dpy == NULL)
      {
      fprintf(stderr,"X Windows failure\n");
      exit(1);
      } /* if X Windows is not active */

   assert(xx->dpy);

   /* get dimensions of root window */
   xx->rootww = XDisplayWidth(xx->dpy,0);
   xx->rootwh = XDisplayHeight(xx->dpy,0);

   /* make display window smaller than root window */
   /* allow for menu bar on top */
   xx->dpywdth = xx->rootww -  80;
   xx->dpyhght = xx->rootwh - 100;

   xx->whiteColor = WhitePixel(xx->dpy, DefaultScreen(xx->dpy));
   xx->blackColor = BlackPixel(xx->dpy, DefaultScreen(xx->dpy));

   xx->w = XCreateSimpleWindow(xx->dpy,
      DefaultRootWindow(xx->dpy),
      0, 0, 
      xx->dpywdth, xx->dpyhght,
      0, xx->whiteColor,
      xx->whiteColor);

   XSelectInput(xx->dpy, xx->w, StructureNotifyMask);

   XMapWindow(xx->dpy, xx->w);

   xx->gc = XCreateGC(xx->dpy, xx->w, 0, NULL);

   xx->fontid = (Font) XLoadFont(xx->dpy,"12x24");

   XSetFont(xx->dpy,xx->gc,xx->fontid);

   XSetForeground(xx->dpy, xx->gc, xx->blackColor);

   xx->dfltscr = XDefaultScreenOfDisplay(xx->dpy);
   if (xx->dfltscr == NULL)
      {
      fprintf(stderr,"XDefaultScreenOfDisplay failed\n");
      perror("XDefaultScreenOfDisplay failed");
      exit(1);
      } /* if error */

   xx->cmap = XDefaultColormapOfScreen(xx->dfltscr);

   rslt = XAllocNamedColor(xx->dpy,xx->cmap,"red",
      &xx->scrdef,&xx->exctdef);

   if (rslt < 0)
      {
      fprintf(stderr,"XAllocNamedColor failed\n");
      perror("XAllocNamedColor failed");
      exit(1);
      } /* if error */
   xx->red = xx->scrdef.pixel;

   rslt = XAllocNamedColor(xx->dpy,xx->cmap,"blue",
      &xx->scrdef,&xx->exctdef);

   if (rslt < 0)
      {
      fprintf(stderr,"XAllocNamedColor failed\n");
      perror("XAllocNamedColor failed");
      exit(1);
      } /* if error */
   xx->blue = xx->scrdef.pixel;

   XSetWindowBorderWidth(xx->dpy, xx->w, 40);

   sprintf(title,"Analog Clock");
   XStoreName(xx->dpy,xx->w,title);
   XSetIconName(xx->dpy,xx->w,title);

   while(1)
      {
      XEvent e;
      XNextEvent(xx->dpy, &e);
      if (e.type == MapNotify) break;
      } /* wait for window initialization */

   } /* initx */

/***********************************************************/
/* clock loop                                              */
/* This loop terminates by pressing 'q'                    */
/***********************************************************/
void runclk(xxfmt *xx)
   {
   int i;                    /* loop counter */
   time_t now;               /* seconds since the epoch */
   struct tm *t;             /* time structure */
   /* Break loop by pressing 'q' */
   while (xx->runflg)
      {
      double theta;          /* angle of clock hands and ticks */
      double sinx;           /* x coordinate */
      double cosx;           /* y coordinate */
      double dblss;          /* current seconds */
      double dblmm;          /* current minutes */
      double dblhh;          /* current hour    */
      char str[256];         /* string for printing text */
      /* get date & time */
      time(&now);                /* seconds since the epoch */
      t = localtime(&now);       /* convert seconds to time structure */
      /***********************************************************/
      /* move time of day to xx structure                        */
      /***********************************************************/
      xx->hh = t->tm_hour;
      xx->mm = t->tm_min;
      xx->ss = t->tm_sec;
      /***********************************************************/
      /* floating point time of day for calculating angles       */
      /***********************************************************/
      dblss = (double) xx->ss;
      dblmm = (double) xx->mm;
      dblhh = (double) (xx->hh % 12);
      /***********************************************************/
      /* clear screen                                            */
      /***********************************************************/
      XSetForeground(xx->dpy, xx->gc, xx->whiteColor);
      XFillRectangle(xx->dpy,xx->w,xx->gc, 800,50,370,350);
      XFillRectangle(xx->dpy,xx->w,xx->gc, 0,0,700,700);
      /***********************************************************/
      /* draw rectangle around text area                         */
      /***********************************************************/
      XSetForeground(xx->dpy, xx->gc, xx->blackColor);
      XDrawRectangle(xx->dpy,xx->w,xx->gc, 800,50,370,350);
      /**************************************************/
      /* print text for digital time and date           */
      /**************************************************/
      sprintf(str,"%02d:%02d:%02d            ",
         xx->hh, xx->mm, xx->ss);
      XDrawString(xx->dpy,xx->w,xx->gc,850,100,str,16);
      /*------------------------------------------------*/
      sprintf(str,"%4d.%02d.%02d  DoY %03d   ",
         t->tm_year+1900, t->tm_mon,
	 t->tm_mday, t->tm_yday);
      XDrawString(xx->dpy,xx->w,xx->gc,850,150,str,20);
      /*------------------------------------------------*/
      sprintf(str,"%s              ",
         xx->weekday+(t->tm_wday<<4));
      XDrawString(xx->dpy,xx->w,xx->gc,850,200,str,20);
      /*------------------------------------------------*/
      if (!t->tm_isdst)
         sprintf(str,"Standard Time             ");
      else
         sprintf(str,"Daylight Saving Time      ");
      XDrawString(xx->dpy,xx->w,xx->gc,850,250,str,20);
      /**************************************************/
      /* draw clock circle                              */
      /**************************************************/
      XDrawArc(xx->dpy,xx->w,xx->gc,50,50,
         600,600,0,360*64);
      /**************************************************/
      /* draw the ticks around the clock                */
      /* draw each 5 minute tick in red                 */
      /**************************************************/
      for (i=0; i<60; i++)
         {
	 double dbltk;
         dbltk = (double) i;
         theta = HALFPI + (dbltk*xx->tick);
	 if (i%5 == 0)
	    {
            XSetForeground(xx->dpy, xx->gc, xx->red);
            sinx  = sin(theta) * 280;
            cosx  = cos(theta) * 280;
	    } /* if 5,10,15,... */
	 else
	    {
            sinx  = sin(theta) * 295;
            cosx  = cos(theta) * 295;
	    } /* else not 5,10,15,... */
         xx->x1 = (int) (50.0 + (300.0 - cosx));
         xx->y1 = (int) (50.0 + (300.0 - sinx));
         sinx  = sin(theta) * 300;
         cosx  = cos(theta) * 300;
         xx->x2 = (int) (50.0 + (300.0 - cosx));
         xx->y2 = (int) (50.0 + (300.0 - sinx));
         XDrawLine(xx->dpy,xx->w,xx->gc,
            xx->x1,xx->y1,xx->x2,xx->y2);
         XSetForeground(xx->dpy, xx->gc, xx->blackColor);
	 } /* for each tick on clock */
      /**************************************************/
      /* draw the arms                                  */
      /**************************************************/
      xx->x1 = xx->y1 = 350;        /* origin of circle */
      /**************************************************/
      /* second hand in red                             */
      /**************************************************/
      theta = HALFPI + (dblss*xx->tick);
      sinx  = sin(theta) * 300;
      cosx  = cos(theta) * 300;
      xx->x2 = (int) (50.0 + (300.0 - cosx));
      xx->y2 = (int) (50.0 + (300.0 - sinx));
      XSetForeground(xx->dpy, xx->gc, xx->red);
      XDrawLine(xx->dpy,xx->w,xx->gc,
         xx->x1,xx->y1,xx->x2,xx->y2);
      /**************************************************/
      /* minute hand                                    */
      /**************************************************/
      theta = HALFPI + (dblmm*xx->tick)
         + (dblss*xx->tick/60.0);
      sinx  = sin(theta) * 300;
      cosx  = cos(theta) * 300;
      xx->x2 = (int) (50.0 + (300.0 - cosx));
      xx->y2 = (int) (50.0 + (300.0 - sinx));
      XSetForeground(xx->dpy, xx->gc, xx->blackColor);
      XDrawLine(xx->dpy,xx->w,xx->gc,
         xx->x1,xx->y1,xx->x2,xx->y2);
      /**************************************************/
      /* hour hand                                      */
      /**************************************************/
      theta = HALFPI + (dblhh*xx->tick*5.0)
         + (dblmm*xx->tick/12.0);
      sinx  = sin(theta) * 200;
      cosx  = cos(theta) * 200;
      xx->x2 = (int) (50.0 + (300.0 - cosx));
      xx->y2 = (int) (50.0 + (300.0 - sinx));
      XDrawLine(xx->dpy,xx->w,xx->gc,
         xx->x1,xx->y1,xx->x2,xx->y2);
      /**************************************************/
      /* check keyboard interrupt                       */
      /**************************************************/
      ifkey(xx);
      if (!xx->runflg) break;         /* 'q' to quit */
      sleep(1);                       /* sleep one second */
      } /* for each second */
   } /* runclk */

int main()
   {
   xxfmt *xx;           /* structure for X Windows data */
   /* allocate memory for the X Windows structure */
   xx = (xxfmt *) malloc(sizeof(xxfmt));
   if (xx == NULL)
      {
      fprintf(stderr,"main: out of memory "
         "allocating xx\n");
      exit(1);
      } /* out of mem */
   /* Start X Windows and open up a canvas (screen) */
   initx(xx);
   /* default foreground color is black */
   XSetForeground(xx->dpy, xx->gc, xx->blackColor);
   xx->tick = SEC;         /* one second in radians */
   /* day of week list */
   strcpy(xx->weekday,"Sunday");
   strcpy(xx->weekday+16,"Monday");
   strcpy(xx->weekday+32,"Tuesday");
   strcpy(xx->weekday+48,"Wednesday");
   strcpy(xx->weekday+64,"Thursday");
   strcpy(xx->weekday+80,"Friday");
   strcpy(xx->weekday+96,"Saturday");
   strcpy(xx->weekday+112,"Sunday");
   xx->runflg = 1;            /* 1=run 0=quit */
   /* this loop is redundant */
   while (xx->runflg)
      {
      runclk(xx);
      ifkey(xx);        /* redundant keyboard check */
      } /* while runflg != 0 */
   /****************************************************/
   /* valgrind likes me to free memory,                */
   /* but I get a segmentation error when I do.        */
   /****************************************************/
   return(0);              /* normal end of job */
   } /* main */
