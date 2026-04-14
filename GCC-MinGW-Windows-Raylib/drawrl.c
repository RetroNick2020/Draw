/*
   drawrl.c  -  QBASIC/QB64PE compatible DRAW command
                   raylib port for GCC/MinGW on Windows
   ======================================================
   Released April 2026 by RetroNick2020

   Build with the supplied Makefile, or manually:
     gcc -o testrl.exe testrl.c drawrl.c ^
         -I./include -L./lib -lraylib -lopengl32 -lgdi32 -lwinmm

   Please visit https://retronick2020.itch.io/vecdraw-v10-for-qbasicgwbasicqb64pe
   Download VecDraw to draw fancy graphics/text and import SVG's to create
   stunning graphics.  Everything exports to Draw statements.  VecDraw is FREE.

   Visit https://retronick2020.itch.io for other great programming utilities.

   PORTING NOTES (OpenWatcom graph.h -> raylib)
   --------------------------------------------
   _setvideomode()          -> InitWindow() / draw_init() / draw_begin()
   _setpixel(x,y)           -> DrawPixel(x,y,col)  [inside BeginTextureMode]
   _setcolor(c)             -> Color resolved from QB palette table
   _floodfill(x,y,border)   -> custom stack-based CPU flood fill (see below)
   _getvideoconfig()        -> GetScreenWidth() / GetScreenHeight()
   short color index        -> Color struct (RGBA) via qb_palette[16]
   getch()                  -> WaitForKeyUp() / custom key wait helper

   FLOOD FILL STRATEGY
   -------------------
   raylib has no built-in flood fill, and reading back pixels from a live
   RenderTexture mid-draw is expensive and fragile.  Instead this library
   maintains a shadow CPU pixel buffer (pixel_buf) that mirrors every pixel
   written via set_pixel() and draw_line().  The P command uses this buffer
   for colour comparison and writes to BOTH the buffer and the GPU canvas,
   keeping them in sync with zero readback cost.

   RENDER TEXTURE Y-AXIS
   ---------------------
   raylib RenderTexture2D stores rows bottom-up (OpenGL convention).  To
   display it right-side-up, use a negative height in DrawTextureRec:
     DrawTextureRec(canvas.texture,
                    (Rectangle){0, 0, canvas_w, -(float)canvas_h},
                    (Vector2){0, 0}, WHITE);
   The shadow pixel_buf uses the normal top-down convention so (0,0) is
   top-left, matching the DRAW coordinate system.
*/

#include "raylib.h"
#include <math.h>
#include <string.h>
#include <ctype.h>
#include <stdlib.h>
#include "drawrl.h"

#ifndef M_PI
#define M_PI 3.14159265358979323846
#endif

/* -----------------------------------------------------------------------
   QB/CGA 16-colour palette -> raylib Color (RGBA)
   Index matches QBASIC COLOR / DRAW C command values.
   ----------------------------------------------------------------------- */
static const Color qb_palette[16] = {
    {  0,   0,   0, 255},   /*  0  Black         */
    {  0,   0, 170, 255},   /*  1  Dark Blue      */
    {  0, 170,   0, 255},   /*  2  Dark Green     */
    {  0, 170, 170, 255},   /*  3  Dark Cyan      */
    {170,   0,   0, 255},   /*  4  Dark Red       */
    {170,   0, 170, 255},   /*  5  Dark Magenta   */
    {170,  85,   0, 255},   /*  6  Brown          */
    {170, 170, 170, 255},   /*  7  Light Gray     */
    { 85,  85,  85, 255},   /*  8  Dark Gray      */
    { 85,  85, 255, 255},   /*  9  Blue           */
    { 85, 255,  85, 255},   /* 10  Green          */
    { 85, 255, 255, 255},   /* 11  Cyan           */
    {255,  85,  85, 255},   /* 12  Red            */
    {255,  85, 255, 255},   /* 13  Magenta        */
    {255, 255,  85, 255},   /* 14  Yellow         */
    {255, 255, 255, 255},   /* 15  White          */
};

/* -----------------------------------------------------------------------
   Canvas / shadow buffer state
   ----------------------------------------------------------------------- */
static RenderTexture2D canvas;
static int    canvas_w   = 0;
static int    canvas_h   = 0;
static Color *pixel_buf  = NULL;   /* CPU mirror: pixel_buf[y*w + x] */

/* -----------------------------------------------------------------------
   Persistent DRAW state across draw() calls
   ----------------------------------------------------------------------- */
static double draw_pos_x      = 0.0;
static double draw_pos_y      = 0.0;
static int    draw_color_idx  = 15;   /* QB colour index 0-15 */
static double draw_scale      = 1.0;  /* S/4, S4 = 1.0 */
static double draw_ta         = 0.0;  /* turn angle in degrees */
static int    draw_initialized = 0;

/* -----------------------------------------------------------------------
   Lifecycle
   ----------------------------------------------------------------------- */
void draw_init(int w, int h)
{
    canvas_w   = w;
    canvas_h   = h;
    canvas     = LoadRenderTexture(w, h);

    /* Allocate CPU shadow buffer, cleared to black (palette index 0) */
    pixel_buf  = (Color *)calloc(w * h, sizeof(Color));

    /* Centre start position */
    draw_pos_x      = w / 2.0;
    draw_pos_y      = h / 2.0;
    draw_initialized = 1;
}

void draw_cleanup(void)
{
    UnloadRenderTexture(canvas);
    free(pixel_buf);
    pixel_buf        = NULL;
    canvas_w         = 0;
    canvas_h         = 0;
    draw_initialized = 0;
}

void draw_begin(void)
{
    BeginTextureMode(canvas);
}

void draw_end(void)
{
    EndTextureMode();
}

RenderTexture2D draw_get_canvas(void)
{
    return canvas;
}

/* -----------------------------------------------------------------------
   Accessors
   ----------------------------------------------------------------------- */
double draw_get_x(void)               { return draw_pos_x; }
double draw_get_y(void)               { return draw_pos_y; }
void   draw_set_pos(double ax, double ay) {
    draw_pos_x = ax;  draw_pos_y = ay;
}
int    draw_get_color(void)           { return draw_color_idx; }
void   draw_set_color(int color)      { draw_color_idx = color & 15; }
double draw_get_scale(void)           { return draw_scale; }
void   draw_set_scale(double scale)   { draw_scale = scale; }
double draw_get_angle(void)           { return draw_ta; }
void   draw_set_angle(double deg)     { draw_ta = deg; }

Color  draw_qb_color(int index)       { return qb_palette[index & 15]; }

/* -----------------------------------------------------------------------
   Internal helpers
   ----------------------------------------------------------------------- */
static int qb_round(double d)
{
    return (d >= 0.0) ? (int)(d + 0.5) : (int)(d - 0.5);
}

static int colors_equal(Color a, Color b)
{
    return a.r == b.r && a.g == b.g && a.b == b.b && a.a == b.a;
}

/* Write one pixel to both the GPU canvas and the CPU shadow buffer.
   Must be called between draw_begin() / draw_end(). */
static void set_pixel(int x, int y, Color col)
{
    if (x < 0 || x >= canvas_w || y < 0 || y >= canvas_h) return;
    DrawPixel(x, y, col);                       /* GPU (raylib)        */
    pixel_buf[y * canvas_w + x] = col;          /* CPU shadow buffer   */
}

/* Bresenham line - uses set_pixel so both buffers stay in sync. */
static void draw_line(int x1, int y1, int x2, int y2, Color col)
{
    int dx  = abs(x2 - x1);
    int dy  = abs(y2 - y1);
    int sx  = (x1 < x2) ? 1 : -1;
    int sy  = (y1 < y2) ? 1 : -1;
    int err = dx - dy;
    int e2;

    for (;;) {
        set_pixel(x1, y1, col);
        if (x1 == x2 && y1 == y2) break;
        e2 = 2 * err;
        if (e2 > -dy) { err -= dy; x1 += sx; }
        if (e2 <  dx) { err += dx; y1 += sy; }
    }
}

/* -----------------------------------------------------------------------
   Stack-based 4-connected flood fill using the CPU shadow buffer.

   We maintain a heap-allocated stack of (x,y) points and grow it on
   demand.  Spreading uses the CPU buffer so there is no GPU readback.
   Every painted pixel is also written to the GPU canvas via set_pixel().

   Must be called between draw_begin() / draw_end().
   ----------------------------------------------------------------------- */
typedef struct { int x; int y; } FillPt;

static void do_flood_fill(int sx, int sy, Color fill_col, Color border_col)
{
    Color   target;
    int     cap, top;
    FillPt *stk;
    FillPt  p;
    int     x, y;

    if (sx < 0 || sx >= canvas_w || sy < 0 || sy >= canvas_h) return;

    target = pixel_buf[sy * canvas_w + sx];

    /* Nothing to do if we're standing on the border or already filled */
    if (colors_equal(target, border_col)) return;
    if (colors_equal(target, fill_col))   return;

    cap = 8192;
    top = 0;
    stk = (FillPt *)malloc((size_t)cap * sizeof(FillPt));
    if (!stk) return;

    stk[top].x = sx;  stk[top].y = sy;  top++;

    while (top > 0) {
        p = stk[--top];
        x = p.x;  y = p.y;

        if (x < 0 || x >= canvas_w || y < 0 || y >= canvas_h) continue;

        {
            Color cur = pixel_buf[y * canvas_w + x];
            if (colors_equal(cur, border_col)) continue;
            if (colors_equal(cur, fill_col))   continue;
        }

        set_pixel(x, y, fill_col);   /* paint GPU + CPU buffer */

        /* Grow stack if needed before pushing 4 neighbours */
        if (top + 4 >= cap) {
            FillPt *ns;
            cap *= 2;
            ns  = (FillPt *)realloc(stk, (size_t)cap * sizeof(FillPt));
            if (!ns) { free(stk); return; }
            stk = ns;
        }

        stk[top].x = x + 1;  stk[top].y = y;     top++;
        stk[top].x = x - 1;  stk[top].y = y;     top++;
        stk[top].x = x;      stk[top].y = y + 1; top++;
        stk[top].x = x;      stk[top].y = y - 1; top++;
    }

    free(stk);
}

/* -----------------------------------------------------------------------
   Parser context
   ----------------------------------------------------------------------- */
typedef struct {
    char str[256];
    int  pos;
    int  len;
} parse_ctx;

static double parse_number(parse_ctx *ctx, int *valid, int *undefined)
{
    int    negative  = 0;
    int    has_digit = 0;
    double int_part  = 0.0;
    double frac_part = 0.0;
    double frac_div  = 1.0;
    char   c;

    *valid     = 1;
    *undefined = 0;

    while (ctx->pos < ctx->len &&
           (ctx->str[ctx->pos] == ' ' || ctx->str[ctx->pos] == '\t'))
        ctx->pos++;

    if (ctx->pos >= ctx->len) { *undefined = 1; return 0.0; }

    c = ctx->str[ctx->pos];
    if      (c == '-') { negative = 1; ctx->pos++; }
    else if (c == '+') {               ctx->pos++; }

    while (ctx->pos < ctx->len &&
           ctx->str[ctx->pos] >= '0' && ctx->str[ctx->pos] <= '9') {
        int_part  = int_part * 10.0 + (ctx->str[ctx->pos] - '0');
        has_digit = 1;
        ctx->pos++;
    }

    if (ctx->pos < ctx->len && ctx->str[ctx->pos] == '.') {
        ctx->pos++;
        while (ctx->pos < ctx->len &&
               ctx->str[ctx->pos] >= '0' && ctx->str[ctx->pos] <= '9') {
            frac_part = frac_part * 10.0 + (ctx->str[ctx->pos] - '0');
            frac_div *= 10.0;
            has_digit  = 1;
            ctx->pos++;
        }
    }

    if (!has_digit) {
        if (negative) ctx->pos--;
        *undefined = 1;
        return 0.0;
    }

    while (ctx->pos < ctx->len &&
           (ctx->str[ctx->pos] == ' ' || ctx->str[ctx->pos] == '\t'))
        ctx->pos++;

    {
        double result = int_part + frac_part / frac_div;
        return negative ? -result : result;
    }
}

/* -----------------------------------------------------------------------
   Main DRAW parser
   Must be called between draw_begin() / draw_end().
   ----------------------------------------------------------------------- */
void draw(const char *cmd)
{
    parse_ctx ctx;
    int       i, cmdlen;

    double vx, vy;          /* Up direction vector      */
    double hx, hy;          /* Right direction vector   */
    double ex, ey;          /* Up-Right diagonal        */
    double fx, fy;          /* Down-Right diagonal      */

    double px, py;          /* current position         */
    double px2, py2;        /* target position          */
    double xx, yy;          /* movement delta           */
    double d;
    double ir;              /* aspect ratio (1:1)       */
    Color  col;             /* current draw colour      */
    int    prefix_b, prefix_n;
    int    num_valid, num_undef;

    cmdlen = (int)strlen(cmd);
    if (cmdlen == 0) return;
    if (cmdlen > 255) cmdlen = 255;
    for (i = 0; i < cmdlen; i++)
        ctx.str[i] = (char)toupper((unsigned char)cmd[i]);
    ctx.str[cmdlen] = '\0';
    ctx.pos = 0;
    ctx.len = cmdlen;

    /* Auto-init position to canvas centre if draw_init was called first */
    if (!draw_initialized) {
        draw_pos_x       = canvas_w / 2.0;
        draw_pos_y       = canvas_h / 2.0;
        draw_initialized = 1;
    }

    px  = draw_pos_x;
    py  = draw_pos_y;
    col = qb_palette[draw_color_idx & 15];
    ir  = 1.0;

    prefix_b = 0;
    prefix_n = 0;

    /* --- rotation macro --- */
    #define APPLY_ROTATION() do { \
        vx = 0;   vy = -1;       \
        ex = ir;  ey = -1;       \
        hx = ir;  hy =  0;       \
        fx = ir;  fy =  1;       \
        if (draw_ta != 0.0) {    \
            double _rad = draw_ta * (M_PI / 180.0); \
            double _st = sin(_rad), _ct = cos(_rad); \
            double _tx, _ty;     \
            _tx=vx; _ty=vy; vx=_tx*_ct+_ty*_st; vy=_ty*_ct-_tx*_st; \
            _tx=hx; _ty=hy; hx=_tx*_ct+_ty*_st; hy=_ty*_ct-_tx*_st; \
            _tx=ex; _ty=ey; ex=_tx*_ct+_ty*_st; ey=_ty*_ct-_tx*_st; \
            _tx=fx; _ty=fy; fx=_tx*_ct+_ty*_st; fy=_ty*_ct-_tx*_st; \
        } \
    } while(0)

    /* --- direction movement macro --- */
    #define DO_MOVEMENT() do { \
        ctx.pos++; \
        d = parse_number(&ctx, &num_valid, &num_undef); \
        if (!num_valid) goto done; \
        if (num_undef) d = 1.0; \
        xx *= d; yy *= d; \
        xx *= ir; \
        px2 = px + xx * draw_scale; \
        py2 = py + yy * draw_scale; \
        if (!prefix_b) \
            draw_line(qb_round(px), qb_round(py), \
                      qb_round(px2), qb_round(py2), col); \
        if (!prefix_n) { px = px2; py = py2; } \
        prefix_b = 0; prefix_n = 0; \
    } while(0)

    APPLY_ROTATION();

    /* --- main parse loop --- */
    while (ctx.pos < ctx.len) {
        char c = ctx.str[ctx.pos];

        if (c == ' ' || c == '\t' || c == ';') { ctx.pos++; continue; }

        switch (c) {

        case 'U': xx =  vx; yy =  vy; DO_MOVEMENT(); break;
        case 'D': xx = -vx; yy = -vy; DO_MOVEMENT(); break;
        case 'L': xx = -hx; yy = -hy; DO_MOVEMENT(); break;
        case 'R': xx =  hx; yy =  hy; DO_MOVEMENT(); break;
        case 'E': xx =  ex; yy =  ey; DO_MOVEMENT(); break;
        case 'F': xx =  fx; yy =  fy; DO_MOVEMENT(); break;
        case 'G': xx = -ex; yy = -ey; DO_MOVEMENT(); break;
        case 'H': xx = -fx; yy = -fy; DO_MOVEMENT(); break;

        case 'M': {
            int is_relative;
            ctx.pos++;
            while (ctx.pos < ctx.len &&
                   (ctx.str[ctx.pos] == ' ' || ctx.str[ctx.pos] == '\t'))
                ctx.pos++;
            if (ctx.pos >= ctx.len) goto done;

            is_relative = (ctx.str[ctx.pos] == '+' ||
                           ctx.str[ctx.pos] == '-');

            px2 = parse_number(&ctx, &num_valid, &num_undef);
            if (!num_valid || num_undef) goto done;

            if (ctx.pos >= ctx.len || ctx.str[ctx.pos] != ',') goto done;
            ctx.pos++;

            py2 = parse_number(&ctx, &num_valid, &num_undef);
            if (!num_valid || num_undef) goto done;

            if (is_relative) {
                xx  = (px2 * ir) * hx - (py2 * ir) * vx;
                yy  =  px2 * hy  - py2 * vy;
                px2 = px + xx * draw_scale;
                py2 = py + yy * draw_scale;
            }

            if (!prefix_b)
                draw_line(qb_round(px), qb_round(py),
                          qb_round(px2), qb_round(py2), col);
            if (!prefix_n) { px = px2; py = py2; }
            prefix_b = 0; prefix_n = 0;
            break;
        }

        case 'B': prefix_b = 1; ctx.pos++; break;
        case 'N': prefix_n = 1; ctx.pos++; break;

        case 'C':
            ctx.pos++;
            d = parse_number(&ctx, &num_valid, &num_undef);
            if (!num_valid || num_undef) goto done;
            draw_color_idx = (int)d & 15;
            col = qb_palette[draw_color_idx];
            break;

        case 'S':
            ctx.pos++;
            d = parse_number(&ctx, &num_valid, &num_undef);
            if (!num_valid || num_undef) goto done;
            if (d < 0) goto done;
            draw_scale = d / 4.0;
            break;

        case 'A':
            ctx.pos++;
            d = parse_number(&ctx, &num_valid, &num_undef);
            if (!num_valid || num_undef) goto done;
            switch ((int)d) {
                case 0: draw_ta =   0; break;
                case 1: draw_ta =  90; break;
                case 2: draw_ta = 180; break;
                case 3: draw_ta = 270; break;
                default: goto done;
            }
            APPLY_ROTATION();
            break;

        case 'T':
            ctx.pos++;
            while (ctx.pos < ctx.len &&
                   (ctx.str[ctx.pos] == ' ' || ctx.str[ctx.pos] == '\t'))
                ctx.pos++;
            if (ctx.pos >= ctx.len)      goto done;
            if (ctx.str[ctx.pos] != 'A') goto done;
            ctx.pos++;
            d = parse_number(&ctx, &num_valid, &num_undef);
            if (!num_valid || num_undef) goto done;
            draw_ta = d;
            APPLY_ROTATION();
            break;

        /* P - Paint (flood fill) using CPU shadow buffer.
           No GPU readback needed; the shadow buffer is always current. */
        case 'P': {
            int   fill_idx, border_idx;
            Color fill_col, border_col;

            ctx.pos++;
            d = parse_number(&ctx, &num_valid, &num_undef);
            if (!num_valid || num_undef) goto done;
            fill_idx = (int)d & 15;

            if (ctx.pos >= ctx.len || ctx.str[ctx.pos] != ',') goto done;
            ctx.pos++;

            d = parse_number(&ctx, &num_valid, &num_undef);
            if (!num_valid || num_undef) goto done;
            border_idx = (int)d & 15;

            fill_col   = qb_palette[fill_idx];
            border_col = qb_palette[border_idx];

            do_flood_fill(qb_round(px), qb_round(py), fill_col, border_col);

            draw_color_idx = fill_idx;
            col = fill_col;
            break;
        }

        default:
            ctx.pos++;
            break;
        }
    }

done:
    #undef APPLY_ROTATION
    #undef DO_MOVEMENT

    draw_pos_x = px;
    draw_pos_y = py;
}
