/*
   TestOD Released on April 10, 2026 By RetroNick2020 

   This program demonstrates only the what is minimal possible with Draw Command.

   Please visit https://retronick2020.itch.io/vecdraw-v10-for-qbasicgwbasicqb64pe

   Download VecDraw to draw fancy graphics/text and import SVG's to create
   stunning graphics. Everything will export to Draw statements, nothing else
   required. VecDraw is Free softwware.

   Visit https://retronick2020.itch.io for other great programming utilities.
   Compile: wcl -ml testod.c owdraw.c graph.lib
        or:  wcl386 testod.c owdraw.c graph.lib
 */

#include <stdio.h>
#include <conio.h>
#include <graph.h>
#include "owdraw.h"

int main(void)
{
     _setvideomode(_VRES16COLOR);  
    draw_set_pos(0, 0);
    /* Yellow square */
    draw("BM100,100 C14 S8 R50 D50 L50 U50");

    /* Red diamond */
    draw_set_pos(320, 150);
    draw_set_color(12);
    draw("E30 F30 G30 H30");
    /* Move inside then fill with light red, border dark red */
    draw_set_pos(0, 0);
    draw("BM370,180 P12,12");

    /* Cyan rotated square */
    draw("BM500,200 C11 TA45 S4 R40 D40 L40 U40");

    /* Green star using N prefix */
    draw("BM200,350 C10 S8");
    draw("NU20 ND20 NL20 NR20 NE20 NF20 NG20 NH20");

    getch();
    _setvideomode(_DEFAULTMODE);
    return 0;
}
