/*
   testod.c - DJGPP + GRX version
   Released on April 10, 2026  By RetroNick2020
   Ported from OpenWatcom (graph.h) to DJGPP 32-bit DOS (grx20.h).

   Demonstrates the minimal DRAW command feature set.

   Please visit https://retronick2020.itch.io/vecdraw-v10-for-qbasicgwbasicqb64pe

   Download VecDraw to draw fancy graphics/text and import SVG's to create
   stunning graphics. Everything will export to Draw statements, nothing else
   required. VecDraw is Free software.

   Visit https://retronick2020.itch.io for other great programming utilities.

   Build:
     gcc -o testod.exe testod.c grxdraw.c -lgrx20 -lm

   Notes on OpenWatcom -> DJGPP/GRX changes in this file:
     #include <graph.h>         ->  #include <grx20.h>
     "owdraw.h"                 ->  "grxdraw.h"
     _setvideomode(_VRES16COLOR)->  GrSetMode(GR_width_height_color,640,480,16)
     _setvideomode(_DEFAULTMODE)->  GrSetMode(GR_default_text)
     getch()                    ->  GrKeyRead()  (GRX keyboard driver)

   Color palette note:
     In a 16-color VGA mode GRX maps color indices 0-15 directly to the
     standard EGA/VGA palette, so C14 = yellow, C12 = light red, C11 = cyan,
     C10 = light green - identical to the OpenWatcom original.
*/

#include <grx20.h>
#include "grxdraw.h"

int main(void)
{
    /* Set 640x480 16-color VGA graphics mode.
       Equivalent to OpenWatcom's _setvideomode(_VRES16COLOR). */
    GrSetMode(GR_width_height_color, 640, 480, 16);

    draw_set_pos(0, 0);

    /* Yellow square */
    draw("BM100,100 C14 S8 R50 D50 L50 U50");

    /* Red diamond */
    draw_set_pos(320, 150);
    draw_set_color(12);
    draw("E30 F30 G30 H30");

    /* Move inside the diamond then flood-fill: light red / border dark red */
    draw_set_pos(0, 0);
    draw("BM370,180 P12,12");

    /* Cyan rotated square */
    draw("BM500,200 C11 TA45 S4 R40 D40 L40 U40");

    /* Green star using the N (draw-without-move) prefix */
    draw("BM200,350 C10 S8");
    draw("NU20 ND20 NL20 NR20 NE20 NF20 NG20 NH20");

    /* Wait for any keypress, then restore text mode.
       GrKeyRead() replaces getch() from <conio.h>.
       If you link in a conio shim you can use getch() instead. */
    GrKeyRead();

    /* Restore text mode - equivalent to _setvideomode(_DEFAULTMODE). */
    GrSetMode(GR_default_text);

    return 0;
}
