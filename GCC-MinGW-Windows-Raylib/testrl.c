/*
   testrl.c  -  DRAW command demo for raylib / GCC / MinGW (Windows)
   ========================================================================
   Released April 2026 by RetroNick2020

   Build with the supplied Makefile, or manually:
     gcc -o testrl.exe testrl.c drawrl.c ^
         -I./include -L./lib -lraylib -lopengl32 -lgdi32 -lwinmm

   Raylib setup (quick guide)
   --------------------------
   1. Download raylib for MinGW-w64 from https://github.com/raysan5/raylib/releases
      (choose the win64-mingw package, e.g. raylib-5.0_win64_mingw-w64.zip)
   2. Unzip and place:
        include/raylib.h     -> <your project>/include/raylib.h
        lib/libraylib.a      -> <your project>/lib/libraylib.a
   3. Run 'make' (or use the manual gcc line above).

   Please visit https://retronick2020.itch.io/vecdraw-v10-for-qbasicgwbasicqb64pe
   Download VecDraw to draw fancy graphics/text and import SVG's to create
   stunning graphics.  Everything exports to Draw statements.  VecDraw is FREE.

   Visit https://retronick2020.itch.io for other great programming utilities.
*/

#include "raylib.h"
#include "drawrl.h"

#define CANVAS_W  640
#define CANVAS_H  480

int main(void)
{
    /* ----------------------------------------------------------------
       1. Create the window FIRST - raylib needs an OpenGL context
          before draw_init() can create its RenderTexture2D canvas.
       ---------------------------------------------------------------- */
    InitWindow(CANVAS_W, CANVAS_H, "DRAW Command Demo - raylib port");
    SetTargetFPS(60);

    /* ----------------------------------------------------------------
       2. Allocate the draw canvas (must be after InitWindow).
       ---------------------------------------------------------------- */
    draw_init(CANVAS_W, CANVAS_H);

    /* ----------------------------------------------------------------
       3. Draw everything into the canvas ONCE, outside the game loop.
          Wrap all draw() calls between draw_begin() / draw_end().
       ---------------------------------------------------------------- */
    draw_begin();
        ClearBackground(BLACK);   /* raylib fill = QB color 0 */

        /* --- Yellow square --- */
        draw_set_pos(0, 0);
        draw("BM100,100 C14 S8 R50 D50 L50 U50");

        /* --- Red diamond --- */
        draw_set_pos(320, 150);
        draw_set_color(12);
        draw("E30 F30 G30 H30");

        /* Move inside the diamond then flood-fill:
           fill colour = 12 (red), border colour = 12 (red) */
        draw_set_pos(0, 0);
        draw("BM370,180 P12,12");

        /* --- Cyan rotated square (TA45 = 45 degrees) --- */
        draw("BM500,200 C11 TA45 S4 R40 D40 L40 U40");

        /* --- Green star using N (no-advance) prefix --- */
        draw("BM200,350 C10 S8");
        draw("NU20 ND20 NL20 NR20 NE20 NF20 NG20 NH20");

        /* --- White text label (raylib DrawText inside BeginTextureMode) --- */
        DrawText("DRAW command - raylib port", 10, 10, 12, WHITE);
        DrawText("Press any key or close window to exit", 10, CANVAS_H - 22, 10, GRAY);

    draw_end();

    /* ----------------------------------------------------------------
       4. Game loop: blit canvas to screen each frame.
          The negative height in DrawTextureRec flips the RenderTexture
          from OpenGL bottom-up convention back to screen top-down.
       ---------------------------------------------------------------- */
    while (!WindowShouldClose() && !GetKeyPressed())
    {
        BeginDrawing();
            ClearBackground(BLACK);

            DrawTextureRec(
                draw_get_canvas().texture,
                (Rectangle){ 0, 0, (float)CANVAS_W, -(float)CANVAS_H },
                (Vector2){ 0, 0 },
                WHITE
            );

        EndDrawing();
    }

    /* ----------------------------------------------------------------
       5. Clean up in reverse order.
       ---------------------------------------------------------------- */
    draw_cleanup();
    CloseWindow();

    return 0;
}
