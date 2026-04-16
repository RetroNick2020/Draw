/*
 * grxdraw.c - QBASIC/QB64PE compatible DRAW command for DJGPP + GRX
 * ==================================================================
 * Converted from owdraw.c (OpenWatcom + graph.h) to DJGPP + GRX 2.x.
 *
 * OpenWatcom  ->  GRX equivalent substitutions:
 *   graph.h              ->  grx20.h
 *   short  (color)       ->  GrColor
 *   _setpixel(x,y)       ->  GrPlot(x, y, color)
 *   _setcolor(c)         ->  (color passed directly to GrPlot / GrFloodFill)
 *   _floodfill(x,y,bdr)  ->  GrFloodFill(x, y, border_color, fill_color)
 *   _getvideoconfig(&vc) ->  GrSizeX() / GrSizeY()
 *
 * NOTE on draw_line():
 *   We keep the Bresenham pixel-by-pixel loop (using GrPlot) rather than
 *   switching to GrLine(), so that both endpoints are always drawn and the
 *   output matches the QB64PE/QBASIC reference exactly.
 *
 * Build:
 *   gcc -o testod.exe testod.c grxdraw.c -lgrx20 -lm
 */

#include <grx20.h>
#include <math.h>
#include <string.h>
#include <ctype.h>
#include <stdlib.h>
#include "grxdraw.h"

#ifndef M_PI
#define M_PI 3.14159265358979323846
#endif

/* -----------------------------------------------------------------------
   Persistent state across draw() calls
   ----------------------------------------------------------------------- */
static double  draw_pos_x      = 0.0;
static double  draw_pos_y      = 0.0;
static GrColor draw_color      = 15;   /* white in EGA/VGA 16-color palette */
static double  draw_scale      = 1.0;  /* S/4 - default S4 = 1.0 pixel/unit */
static double  draw_ta         = 0.0;  /* current angle in degrees */
static int     draw_initialized = 0;

/* -----------------------------------------------------------------------
   Accessors
   ----------------------------------------------------------------------- */
double  draw_get_x(void)               { return draw_pos_x;  }
double  draw_get_y(void)               { return draw_pos_y;  }

void    draw_set_pos(double ax, double ay)
{
    draw_pos_x = ax;
    draw_pos_y = ay;
    draw_initialized = 1;
}

GrColor draw_get_color(void)           { return draw_color;  }
void    draw_set_color(GrColor color)  { draw_color = color; }

double  draw_get_scale(void)           { return draw_scale;  }
void    draw_set_scale(double scale)   { draw_scale = scale; }

double  draw_get_angle(void)           { return draw_ta;     }
void    draw_set_angle(double deg)     { draw_ta = deg;      }

/* -----------------------------------------------------------------------
   Internal: QB-compatible rounding
   Mirrors QB64PE qbr() / Pascal Trunc(x +/- 0.5).
   ----------------------------------------------------------------------- */
static int qb_round(double d)
{
    if (d >= 0.0)
        return (int)(d + 0.5);
    else
        return (int)(d - 0.5);
}

/* -----------------------------------------------------------------------
   Internal: Bresenham line using GrPlot()
   ----------------------------------------------------------------------- */
static void draw_line(int x1, int y1, int x2, int y2, GrColor color)
{
    int dx, dy, sx, sy, err, e2;

    dx = abs(x2 - x1);
    dy = abs(y2 - y1);
    sx = (x1 < x2) ? 1 : -1;
    sy = (y1 < y2) ? 1 : -1;
    err = dx - dy;

    for (;;) {
        GrPlot(x1, y1, color);          /* replaces _setpixel(x1, y1) */
        if (x1 == x2 && y1 == y2) break;
        e2 = 2 * err;
        if (e2 > -dy) { err -= dy; x1 += sx; }
        if (e2 <  dx) { err += dx; y1 += sy; }
    }
}

/* -----------------------------------------------------------------------
   Internal: parser context and helpers
   ----------------------------------------------------------------------- */
typedef struct {
    char str[256];  /* uppercase copy of the command string */
    int  pos;
    int  len;
} parse_ctx;

static char peek_char(parse_ctx *ctx)
{
    if (ctx->pos >= ctx->len)
        return '\0';
    return ctx->str[ctx->pos];
}

static void skip_ws(parse_ctx *ctx)
{
    while (ctx->pos < ctx->len) {
        char c = ctx->str[ctx->pos];
        if (c == ' ' || c == '\t' || c == ';')
            ctx->pos++;
        else
            break;
    }
}

/* Parse a number from the current position.
   Returns the value; sets *valid=0 on syntax error,
   *undefined=1 when no digits are present (caller should use a default). */
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

    /* skip leading spaces/tabs (not semicolons - those separate commands) */
    while (ctx->pos < ctx->len &&
           (ctx->str[ctx->pos] == ' ' || ctx->str[ctx->pos] == '\t'))
        ctx->pos++;

    if (ctx->pos >= ctx->len) {
        *undefined = 1;
        return 0.0;
    }

    c = ctx->str[ctx->pos];

    /* optional sign */
    if (c == '-') {
        negative = 1;
        ctx->pos++;
    } else if (c == '+') {
        ctx->pos++;
    }

    /* integer part */
    while (ctx->pos < ctx->len &&
           ctx->str[ctx->pos] >= '0' && ctx->str[ctx->pos] <= '9') {
        int_part = int_part * 10.0 + (ctx->str[ctx->pos] - '0');
        has_digit = 1;
        ctx->pos++;
    }

    /* optional fractional part */
    if (ctx->pos < ctx->len && ctx->str[ctx->pos] == '.') {
        ctx->pos++;
        while (ctx->pos < ctx->len &&
               ctx->str[ctx->pos] >= '0' && ctx->str[ctx->pos] <= '9') {
            frac_part = frac_part * 10.0 + (ctx->str[ctx->pos] - '0');
            frac_div *= 10.0;
            has_digit = 1;
            ctx->pos++;
        }
    }

    if (!has_digit) {
        /* consumed a sign but no digits: back up the '-' */
        if (negative)
            ctx->pos--;
        *undefined = 1;
        return 0.0;
    }

    /* skip trailing spaces/tabs */
    while (ctx->pos < ctx->len &&
           (ctx->str[ctx->pos] == ' ' || ctx->str[ctx->pos] == '\t'))
        ctx->pos++;

    {
        double result = int_part + frac_part / frac_div;
        return negative ? -result : result;
    }
}

/* -----------------------------------------------------------------------
   Main DRAW parser / executor
   ----------------------------------------------------------------------- */
void draw(const char *cmd)
{
    parse_ctx ctx;
    int       i, cmdlen;

    /* direction vectors */
    double vx, vy;      /* Up         */
    double hx, hy;      /* Right      */
    double ex, ey;      /* Up-Right   */
    double fx, fy;      /* Down-Right */

    double px, py;      /* current position  */
    double px2, py2;    /* target  position  */
    double xx, yy;      /* movement delta    */
    double d;
    double ir;          /* aspect ratio - always 1.0 for GRX/square pixels */
    GrColor col;
    int     prefix_b, prefix_n;
    int     num_valid, num_undef;

    /* uppercase copy of the command string */
    cmdlen = (int)strlen(cmd);
    if (cmdlen == 0) return;
    if (cmdlen > 255) cmdlen = 255;
    for (i = 0; i < cmdlen; i++)
        ctx.str[i] = (char)toupper((unsigned char)cmd[i]);
    ctx.str[cmdlen] = '\0';
    ctx.pos = 0;
    ctx.len = cmdlen;

    /* first call: centre on screen using GRX screen dimensions */
    if (!draw_initialized) {
        draw_pos_x = GrSizeX() / 2;    /* replaces vc.numxpixels / 2 */
        draw_pos_y = GrSizeY() / 2;    /* replaces vc.numypixels / 2 */
        draw_initialized = 1;
    }

    px  = draw_pos_x;
    py  = draw_pos_y;
    col = draw_color;
    ir  = 1.0;

    prefix_b = 0;
    prefix_n = 0;

    /* ---- macro: (re)build direction vectors and rotate by draw_ta ---- */
    #define APPLY_ROTATION() do {                                           \
        vx = 0.0;  vy = -1.0;                                             \
        ex = ir;   ey = -1.0;                                             \
        hx = ir;   hy =  0.0;                                             \
        fx = ir;   fy =  1.0;                                             \
        if (draw_ta != 0.0) {                                             \
            double rad = draw_ta * (M_PI / 180.0);                        \
            double st = sin(rad), ct = cos(rad);                          \
            double tx, ty;                                                \
            tx=vx; ty=vy; vx=tx*ct+ty*st; vy=ty*ct-tx*st;               \
            tx=hx; ty=hy; hx=tx*ct+ty*st; hy=ty*ct-tx*st;               \
            tx=ex; ty=ey; ex=tx*ct+ty*st; ey=ty*ct-tx*st;               \
            tx=fx; ty=fy; fx=tx*ct+ty*st; fy=ty*ct-tx*st;               \
        }                                                                 \
    } while(0)

    /* ---- macro: execute a directional move (U/D/L/R/E/F/G/H) ---- */
    #define DO_MOVEMENT() do {                                              \
        ctx.pos++;                                                         \
        d = parse_number(&ctx, &num_valid, &num_undef);                   \
        if (!num_valid) goto done;                                        \
        if (num_undef) d = 1.0;                                           \
        xx *= d;  yy *= d;                                                \
        xx *= ir;                                                         \
        px2 = px + xx * draw_scale;                                       \
        py2 = py + yy * draw_scale;                                       \
        if (!prefix_b)                                                    \
            draw_line(qb_round(px), qb_round(py),                        \
                      qb_round(px2), qb_round(py2), col);                \
        if (!prefix_n) { px = px2; py = py2; }                           \
        prefix_b = 0;  prefix_n = 0;                                     \
    } while(0)

    APPLY_ROTATION();

    /* ---- main parse loop ---- */
    while (ctx.pos < ctx.len) {
        char c = ctx.str[ctx.pos];

        /* skip whitespace and semicolons between tokens */
        if (c == ' ' || c == '\t' || c == ';') {
            ctx.pos++;
            continue;
        }

        switch (c) {

        /* --- directional movement --- */
        case 'U': xx =  vx; yy =  vy; DO_MOVEMENT(); break;
        case 'D': xx = -vx; yy = -vy; DO_MOVEMENT(); break;
        case 'L': xx = -hx; yy = -hy; DO_MOVEMENT(); break;
        case 'R': xx =  hx; yy =  hy; DO_MOVEMENT(); break;
        case 'E': xx =  ex; yy =  ey; DO_MOVEMENT(); break;
        case 'F': xx =  fx; yy =  fy; DO_MOVEMENT(); break;
        case 'G': xx = -ex; yy = -ey; DO_MOVEMENT(); break;
        case 'H': xx = -fx; yy = -fy; DO_MOVEMENT(); break;

        /* --- M: move to absolute or relative position --- */
        case 'M': {
            int is_relative;
            ctx.pos++;

            /* skip whitespace after 'M' */
            while (ctx.pos < ctx.len &&
                   (ctx.str[ctx.pos] == ' ' || ctx.str[ctx.pos] == '\t'))
                ctx.pos++;
            if (ctx.pos >= ctx.len) goto done;

            /* leading +/- means relative movement */
            is_relative = (ctx.str[ctx.pos] == '+' ||
                           ctx.str[ctx.pos] == '-');

            px2 = parse_number(&ctx, &num_valid, &num_undef);
            if (!num_valid || num_undef) goto done;

            if (ctx.pos >= ctx.len || ctx.str[ctx.pos] != ',') goto done;
            ctx.pos++;

            py2 = parse_number(&ctx, &num_valid, &num_undef);
            if (!num_valid || num_undef) goto done;

            if (is_relative) {
                /* apply rotation & scale - mirrors QB64PE behaviour */
                xx  = (px2 * ir) * hx - (py2 * ir) * vx;
                yy  =  px2 * hy  -  py2 * vy;
                px2 = px + xx * draw_scale;
                py2 = py + yy * draw_scale;
            }

            if (!prefix_b)
                draw_line(qb_round(px),  qb_round(py),
                          qb_round(px2), qb_round(py2), col);
            if (!prefix_n) { px = px2; py = py2; }
            prefix_b = 0;
            prefix_n = 0;
            break;
        }

        /* --- B: move-without-draw prefix --- */
        case 'B':
            prefix_b = 1;
            ctx.pos++;
            break;

        /* --- N: draw-without-move prefix --- */
        case 'N':
            prefix_n = 1;
            ctx.pos++;
            break;

        /* --- C: set drawing color --- */
        case 'C':
            ctx.pos++;
            d = parse_number(&ctx, &num_valid, &num_undef);
            if (!num_valid || num_undef) goto done;
            col        = (GrColor)(int)d;
            draw_color = col;
            break;

        /* --- S: set scale factor (S4 = 1 pixel per unit) --- */
        case 'S':
            ctx.pos++;
            d = parse_number(&ctx, &num_valid, &num_undef);
            if (!num_valid || num_undef) goto done;
            if (d < 0.0) goto done;
            draw_scale = d / 4.0;
            break;

        /* --- A: set angle by index (0=0, 1=90, 2=180, 3=270) --- */
        case 'A':
            ctx.pos++;
            d = parse_number(&ctx, &num_valid, &num_undef);
            if (!num_valid || num_undef) goto done;
            switch ((int)d) {
                case 0: draw_ta =   0.0; break;
                case 1: draw_ta =  90.0; break;
                case 2: draw_ta = 180.0; break;
                case 3: draw_ta = 270.0; break;
                default: goto done;
            }
            APPLY_ROTATION();
            break;

        /* --- T: must be TA (turn angle in degrees) --- */
        case 'T':
            ctx.pos++;
            /* skip optional whitespace between 'T' and 'A' */
            while (ctx.pos < ctx.len &&
                   (ctx.str[ctx.pos] == ' ' || ctx.str[ctx.pos] == '\t'))
                ctx.pos++;
            if (ctx.pos >= ctx.len) goto done;
            if (ctx.str[ctx.pos] != 'A') goto done;   /* only TA is valid */
            ctx.pos++;
            d = parse_number(&ctx, &num_valid, &num_undef);
            if (!num_valid || num_undef) goto done;
            draw_ta = d;
            APPLY_ROTATION();
            break;

        /* --- P: paint / flood fill ---
             OpenWatcom: _setcolor(fill); _floodfill(x, y, border);
             GRX:        GrFloodFill(x, y, border_color, fill_color);   */
        case 'P': {
            GrColor fill_col, border_col;
            ctx.pos++;

            d = parse_number(&ctx, &num_valid, &num_undef);
            if (!num_valid || num_undef) goto done;
            fill_col = (GrColor)(int)d;

            if (ctx.pos >= ctx.len || ctx.str[ctx.pos] != ',') goto done;
            ctx.pos++;

            d = parse_number(&ctx, &num_valid, &num_undef);
            if (!num_valid || num_undef) goto done;
            border_col = (GrColor)(int)d;

            /* GrFloodFill(x, y, border_color, fill_color)             */
            GrFloodFill(qb_round(px), qb_round(py), border_col, fill_col);
            col        = fill_col;
            draw_color = col;
            break;
        }

        /* --- unknown token: skip --- */
        default:
            ctx.pos++;
            break;
        }
    }

done:
    #undef APPLY_ROTATION
    #undef DO_MOVEMENT

    /* persist updated position */
    draw_pos_x = px;
    draw_pos_y = py;
    draw_initialized = 1;
}
