/*
   testtc.c  -  DRAW command demo, Turbo C v2 or Borland C++ 3.1 (BGI graphics)
   ==============================================================
   Released April 2026 by RetroNick2020

   Compile with:
   Turbo C v2.01  
   tcc -ml testtc.c drawtc.c graphics.lib
   
   Borland C++
   bcc -ml testtc.c drawtc.c graphics.lib
     
   NOTE: BGI driver path passed to initgraph() is "" which tells Turbo C
   to look in the current directory.  If you installed Turbo C to C:\TC,
   pass "C:\\TC\\BGI" instead.

   Please visit https://retronick2020.itch.io/vecdraw-v10-for-qbasicgwbasicqb64pe
   Download VecDraw to draw fancy graphics/text and import SVG's to create
   stunning graphics.  Everything exports to Draw statements.  VecDraw is FREE.

   Visit https://retronick2020.itch.io for other great programming utilities.
*/

#include <graphics.h>
#include <conio.h>
#include <stdio.h>
#include "drawtc.h"

int main(void)
{
    int gd = DETECT, gm;  /*  int gd = VGA, gm = VGAHI; */
    char bgi_path[80];

    /* -----------------------------------------------------------------
       Initialise BGI.  Pass the path to your BGI driver files.
       "" = current directory.  Adjust if needed, e.g. "C:\\TC\\BGI"
       ----------------------------------------------------------------- */
    bgi_path[0] = '\0';            /* search current directory */
    initgraph(&gd, &gm, bgi_path);

    if (graphresult() != grOk) {
        printf("BGI init failed: %s\n", grapherrormsg(graphresult()));
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
    draw("BM200,350 C10 S8");
    draw("NU20 ND20 NL20 NR20 NE20 NF20 NG20 NH20");

    getch();

    closegraph();    /* BGI: restore text mode and free resources */
    return 0;
}
