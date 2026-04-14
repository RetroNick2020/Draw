/*
   drawrl.h  -  QBASIC/QB64PE compatible DRAW command
                   raylib port for GCC/MinGW on Windows
   ======================================================
   Released April 2026 by RetroNick2020
   Ported to raylib (https://www.raylib.com).

   Build with the supplied Makefile, or manually:
     gcc -o testrl.exe testrl.c drawrl.c ^
         -I./include -L./lib -lraylib -lopengl32 -lgdi32 -lwinmm

   Please visit https://retronick2020.itch.io/vecdraw-v10-for-qbasicgwbasicqb64pe
   Download VecDraw to draw fancy graphics/text and import SVG's to create
   stunning graphics.  Everything exports to Draw statements.  VecDraw is FREE.

   Visit https://retronick2020.itch.io for other great programming utilities.

   USAGE OVERVIEW
   --------------
   1. Call InitWindow() first (raylib must have an OpenGL context before
      draw_init() can create its internal RenderTexture2D canvas).
   2. Call draw_init(width, height) once to allocate the canvas.
   3. Wrap all draw() calls between draw_begin() / draw_end().
   4. In your render loop use draw_get_canvas() to blit the canvas to screen.
   5. Call draw_cleanup() before CloseWindow().

   DRAW COMMAND REFERENCE
   ----------------------
   U/D/L/R/E/F/G/H [n]   Move/draw in 8 directions n pixels (default 1)
   M [+|-]x,y             Move/draw to absolute (or relative) coords
   B                      Prefix: blind move (no line drawn)
   N                      Prefix: no advance (position unchanged after draw)
   C n                    Set colour (0-15, QB CGA/EGA palette)
   S n                    Set scale factor (S4 = 1:1 default)
   A n                    Set angle preset (0=0 deg, 1=90, 2=180, 3=270)
   TA n                   Turn angle in degrees (-360..360)
   P fill,border          Flood fill at current position
*/

#include "raylib.h"

/* -----------------------------------------------------------------------
   Lifecycle - call in this order:
     InitWindow(...)  <- raylib must be up first
     draw_init(w, h)
     ... game loop ...
       draw_begin()
         draw(...)
       draw_end()
       BeginDrawing()
         DrawTextureRec(draw_get_canvas().texture,
                        (Rectangle){0,0,w,-h}, (Vector2){0,0}, WHITE);
       EndDrawing()
     ... end loop ...
     draw_cleanup()
     CloseWindow()
   ----------------------------------------------------------------------- */
void           draw_init(int canvas_width, int canvas_height);
void           draw_cleanup(void);
void           draw_begin(void);          /* BeginTextureMode wrapper  */
void           draw_end(void);            /* EndTextureMode wrapper    */
RenderTexture2D draw_get_canvas(void);    /* canvas for screen blit   */

/* -----------------------------------------------------------------------
   Main DRAW command - call between draw_begin() / draw_end()
   ----------------------------------------------------------------------- */
void draw(const char *cmd);

/* -----------------------------------------------------------------------
   State accessors
   ----------------------------------------------------------------------- */
double draw_get_x(void);
double draw_get_y(void);
void   draw_set_pos(double ax, double ay);

int    draw_get_color(void);          /* QB color index 0-15 */
void   draw_set_color(int color);

double draw_get_scale(void);
void   draw_set_scale(double scale);

double draw_get_angle(void);
void   draw_set_angle(double deg);

/* -----------------------------------------------------------------------
   Utility: convert a QB color index (0-15) to a raylib Color struct.
   Useful if you need to set the clear background to match a QB color.
   ----------------------------------------------------------------------- */
Color  draw_qb_color(int index);

