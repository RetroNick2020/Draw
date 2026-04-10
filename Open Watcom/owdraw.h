/*
 * owdraw.h - QBASIC/QB64PE compatible DRAW command for OpenWatcom C
 * ==================================================================
 * Implements the classic BASIC DRAW statement using OpenWatcom's graph.h
 *
 * Supported commands:
 *   Movement/Drawing:
 *     U[n]  - Up n units (default 1)
 *     D[n]  - Down n units
 *     L[n]  - Left n units
 *     R[n]  - Right n units
 *     E[n]  - Up-Right (diagonal)
 *     F[n]  - Down-Right (diagonal)
 *     G[n]  - Down-Left (diagonal)
 *     H[n]  - Up-Left (diagonal)
 *     M[+/-]x,y - Move to absolute x,y or relative +x,+y / -x,-y
 *
 *   Prefixes (apply to the next movement command only):
 *     B     - Move without drawing
 *     N     - Draw and return to original position
 *
 *   Color/Style:
 *     Cn    - Set drawing color to n
 *     Sn    - Set scale factor (default 4 = 1 pixel per unit)
 *     An    - Set angle: 0=0, 1=90, 2=180, 3=270 degrees
 *     TAn   - Set angle to n degrees (-360 to 360)
 *     Pf,b  - Paint (flood fill) with fill color f, border color b
 *
 * Whitespace and semicolons are ignored between commands.
 *
 * Usage:
 *   #include <graph.h>
 *   #include "owdraw.h"
 *
 *   _setvideomode(_VRES16COLOR);
 *   draw("BM100,100 C14 S8 R50 D50 L50 U50");
 *   draw("BM160,100 TA45 R50");
 *   _setvideomode(_DEFAULTMODE);
 */

#ifndef OWDRAW_H
#define OWDRAW_H

#ifdef __cplusplus
extern "C" {
#endif

/* Main DRAW command */
void draw(const char *cmd);

/* Get/Set persistent DRAW state */
double draw_get_x(void);
double draw_get_y(void);
void   draw_set_pos(double ax, double ay);
short  draw_get_color(void);
void   draw_set_color(short color);
double draw_get_scale(void);
void   draw_set_scale(double scale);
double draw_get_angle(void);
void   draw_set_angle(double degrees);

#ifdef __cplusplus
}
#endif

#endif /* OWDRAW_H */
