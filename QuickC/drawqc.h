/*
   drawqc.h  -  QBASIC/QB64PE compatible DRAW command
                   Microsoft QuickC 2.1 port
   =====================================================
   Released April 2026 by RetroNick2020
   Ported to Microsoft QuickC 2.1 (Microsoft graph.h).

   Compile with:
     qcl /AL testqc.c drawqc.c graphics.lib
   
   Please visit https://retronick2020.itch.io/vecdraw-v10-for-qbasicgwbasicqb64pe
   Download VecDraw to draw fancy graphics/text and import SVG's to create
   stunning graphics.  Everything exports to Draw statements.  VecDraw is FREE.

   Visit https://retronick2020.itch.io for other great programming utilities.
*/


/* -----------------------------------------------------------------------
   Main draw command - accepts a QBASIC-style DRAW string.
   Supports: U D L R E F G H  (8 directions)
             M x,y            (move/draw to absolute or relative coords)
             B                (blind move prefix - no drawing)
             N                (no-advance prefix - position unchanged)
             C n              (set color 0-15)
             S n              (set scale, S4 = 1:1)
             A n              (set angle: 0=0 deg, 1=90, 2=180, 3=270)
             TA n             (set turn angle in degrees, -360..360)
             P fill,border    (flood fill)
   ----------------------------------------------------------------------- */
void draw(const char *cmd);

/* State accessors - call these before draw() if needed */
double draw_get_x(void);
double draw_get_y(void);
void   draw_set_pos(double ax, double ay);

short  draw_get_color(void);
void   draw_set_color(short color);

double draw_get_scale(void);
void   draw_set_scale(double scale);

double draw_get_angle(void);
void   draw_set_angle(double deg);
