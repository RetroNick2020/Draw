/*
   testqc.c  -  DRAW command demo, Microsoft QuickC 2.1
   ==========================================================
   Released April 2026 by RetroNick2020

   Compile with:
     qcl /AL testqc.c drawqc.c graphics.lib

   VIDEO MODE NOTE:
     _ERESCOLOR  = EGA/VGA 640x350x16  (safe default, works on EGA & VGA)
     _VRES16COLOR= VGA     640x480x16  (change to this if you have VGA)

   Please visit https://retronick2020.itch.io/vecdraw-v10-for-qbasicgwbasicqb64pe
   Download VecDraw to draw fancy graphics/text and import SVG's to create
   stunning graphics.  Everything exports to Draw statements.  VecDraw is FREE.

   Visit https://retronick2020.itch.io for other great programming utilities.
*/

#include <graph.h>
#include <conio.h>
#include <stdio.h>
#include "drawqc.h"

int main(void)
{
    /* Use _ERESCOLOR (EGA 640x350x16) for broadest compatibility.
       Switch to _VRES16COLOR for VGA 640x480x16 if your adapter supports it. */
    if (_setvideomode(_VRES16COLOR) == 0) {
        printf("Cannot set graphics mode.\n");
        return 1;
    }

    /* --- Yellow square --- */
    draw_set_pos(0, 0);
    draw("BM100,100 C14 S8 R50 D50 L50 U50");

    /* --- Red diamond --- */
    draw_set_pos(320, 150);
    draw_set_color(12);
    draw("E30 F30 G30 H30");

    /* Move inside diamond then flood-fill with color 12, border 12 */
    draw_set_pos(0, 0);
    draw("BM370,180 P12,12");

    /* --- Cyan rotated square (45 degrees) --- */
    draw("BM500,200 C11 TA45 S4 R40 D40 L40 U40");

    /* --- Green star using N (no-advance) prefix --- */
    draw("BM200,300 C10 S8");
    draw("NU20 ND20 NL20 NR20 NE20 NF20 NG20 NH20");

    getch();

    _setvideomode(_DEFAULTMODE);   /* restore text mode */
    return 0;
}
