/*
 * grxdraw.h - QBASIC/QB64PE compatible DRAW command for DJGPP + GRX
 * ==================================================================
 * Converted from owdraw.h (OpenWatcom) by RetroNick2020 port to DJGPP.
 * Uses the GRX 2.x graphics library in place of OpenWatcom graph.h.
 *
 * Supported DRAW command string tokens:
 *   Movement/Drawing:
 *     U[n]  - Up n units (default 1)
 *     D[n]  - Down n units
 *     L[n]  - Left n units
 *     R[n]  - Right n units
 *     E[n]  - Up-Right (diagonal)
 *     F[n]  - Down-Right (diagonal)
 *     G[n]  - Down-Left (diagonal)
 *     H[n]  - Up-Left (diagonal)
 *     M[+/-]x,y  - Move to absolute x,y  or relative +/-x,+/-y
 *
 *   Prefixes (apply to the next movement command only):
 *     B  - Move without drawing
 *     N  - Draw and return to original position
 *
 *   Color / Style:
 *     Cn    - Set drawing color index to n  (0-15 for 16-color modes)
 *     Sn    - Set scale factor  (S4 = 1 pixel per unit, default)
 *     An    - Set angle: 0=0 deg, 1=90, 2=180, 3=270
 *     TAn   - Set angle to exactly n degrees  (-360 to 360)
 *     Pf,b  - Paint (flood fill) fill-color f, border-color b
 *
 * Whitespace and semicolons between tokens are ignored.
 *
 * Build:
 *   gcc -o testod.exe testod.c grxdraw.c -lgrx20 -lm
 *
 * Usage:
 *   #include <grx20.h>
 *   #include "grxdraw.h"
 *
 *   GrSetMode(GR_width_height_color, 640, 480, 16);
 *   draw_set_pos(0, 0);
 *   draw("BM100,100 C14 S8 R50 D50 L50 U50");
 *   GrSetMode(GR_default_text);
 */

#ifndef GRXDRAW_H
#define GRXDRAW_H

#include <grx20.h>   /* GrColor type lives here */

#ifdef __cplusplus
extern "C" {
#endif

/* Main DRAW command - parses and executes a DRAW command string */
void   draw(const char *cmd);

/* Get / Set persistent DRAW state */
double   draw_get_x(void);
double   draw_get_y(void);
void     draw_set_pos(double ax, double ay);

GrColor  draw_get_color(void);
void     draw_set_color(GrColor color);

double   draw_get_scale(void);
void     draw_set_scale(double scale);

double   draw_get_angle(void);
void     draw_set_angle(double degrees);

#ifdef __cplusplus
}
#endif

#endif /* GRXDRAW_H */
