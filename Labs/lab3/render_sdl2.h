/*******************************************************************************#
#           guvcview              http://guvcview.sourceforge.net               #
#                                                                               #
#           Paulo Assis <pj.assis@gmail.com>                                    #
#           Nobuhiro Iwamatsu <iwamatsu@nigauri.org>                            #
#                             Add UYVY color support(Macbook iSight)            #
#           Flemming Frandsen <dren.dk@gmail.com>                               #
#                             Add VU meter OSD                                  #
#                                                                               #
# This program is free software; you can redistribute it and/or modify          #
# it under the terms of the GNU General Public License as published by          #
# the Free Software Foundation; either version 2 of the License, or             #
# (at your option) any later version.                                           #
#                                                                               #
# This program is distributed in the hope that it will be useful,               #
# but WITHOUT ANY WARRANTY; without even the implied warranty of                #
# MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the                 #
# GNU General Public License for more details.                                  #
#                                                                               #
# You should have received a copy of the GNU General Public License             #
# along with this program; if not, write to the Free Software                   #
# Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA     #
#                                                                               #
********************************************************************************/

#ifndef RENDER_SDL2_H
#define RENDER_SDL2_H
#include <stdint.h>

/*
 * init sdl2 render
 * args:
 *    width - overlay width
 *    height - overlay height
 *    flags - window flags:
 *              0- none
 *              1- fullscreen
 *              2- maximized
 *
 * asserts:
 *
 * returns: error code (0 ok)
 */
int init_render_sdl2(int width, int height, int flags);

/*
 * render a frame
 * args:
 *   frame - pointer to frame data (yuyv format)
 *   width - frame width
 *   height - frame height
 *
 * asserts:
 *   poverlay is not nul
 *   frame is not null
 *
 * returns: error code
 */
int render_sdl2_frame(uint8_t *frame, int pitch);

/*
 * set sdl1 render caption
 * args:
 *   caption - string with render window caption
 *
 * asserts:
 *   none
 *
 * returns: none
 */
void set_render_sdl2_caption(const char* caption);

/*
 * dispatch sdl1 render events
 * args:
 *   none
 *
 * asserts:
 *   none
 *
 * returns: none
 */
void render_sdl2_dispatch_events();

/*
 * clean sdl1 render data
 * args:
 *   none
 *
 * asserts:
 *   none
 *
 * returns: none
 */
void render_sdl2_clean();



int decode_sdl2_mjpeg_frame(uint8_t *src, uint8_t *dst, size_t size);

int RGB24_to_GREY(uint8_t *src, uint8_t *dst, int imgsize);

int GREY_to_RGB24(uint8_t *src, uint8_t *dst, int imgsize);


enum render_event_callback_enum {
EV_QUIT = 0 ,
EV_KEY_UP   ,
EV_KEY_DOWN ,
EV_KEY_LEFT ,
EV_KEY_RIGHT,
EV_KEY_SPACE,
EV_KEY_I    ,
EV_KEY_V    ,
};

typedef int (*render_event_callback)(void *data);

typedef struct _render_events_t
{
	int id;
	render_event_callback callback;
	void *data;

} render_events_t;


int render_set_event_callback(int id, render_event_callback callback_function, void *data);

int render_call_event_callback(int id);


#endif