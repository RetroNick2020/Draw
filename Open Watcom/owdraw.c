/*
   owdraw.c Released on April 10, 2026 By RetroNick2020 

   This libray demonstrates only what is minimal possible with the Draw Command.

   Please visit https://retronick2020.itch.io/vecdraw-v10-for-qbasicgwbasicqb64pe

   Download VecDraw to draw fancy graphics/text and import SVG's to create
   stunning graphics. Everything will export to Draw statements, nothing else
   required. VecDraw is Free softwware.

   Visit https://retronick2020.itch.io for other great programming utilities.
 
  owdraw.c - QBASIC/QB64PE compatible DRAW command for OpenWatcom C
  ==================================================================
  Compile with: wcl -ml owdraw.c yourprog.c graph.lib
            or: wcl386 owdraw.c yourprog.c graph.lib
*/

#include <graph.h>
#include <math.h>
#include <string.h>
#include <ctype.h>
#include "owdraw.h"

#ifndef M_PI
#define M_PI 3.14159265358979323846
#endif

/* -----------------------------------------------------------------------
   Persistent state across draw() calls
   ----------------------------------------------------------------------- */
static double draw_pos_x   = 0.0;
static double draw_pos_y   = 0.0;
static short  draw_color   = 15;
static double draw_scale   = 1.0;   /* S/4 - default S4 = 1.0 */
static double draw_ta      = 0.0;   /* angle in degrees */
static int    draw_initialized = 0;

/* -----------------------------------------------------------------------
   Accessors
   ----------------------------------------------------------------------- */
double draw_get_x(void)              { return draw_pos_x; }
double draw_get_y(void)              { return draw_pos_y; }
void   draw_set_pos(double ax, double ay) {
    draw_pos_x = ax;
    draw_pos_y = ay;
    draw_initialized = 1;
}
short  draw_get_color(void)          { return draw_color; }
void   draw_set_color(short color)   { draw_color = color; }
double draw_get_scale(void)          { return draw_scale; }
void   draw_set_scale(double scale)  { draw_scale = scale; }
double draw_get_angle(void)          { return draw_ta; }
void   draw_set_angle(double deg)    { draw_ta = deg; }

/* -----------------------------------------------------------------------
   Internal: QB-compatible rounding
   ----------------------------------------------------------------------- */
static int qb_round(double d)
{
    if (d >= 0.0)
        return (int)(d + 0.5);
    else
        return (int)(d - 0.5);
}

/* -----------------------------------------------------------------------
   Internal: Bresenham line using _setpixel
   We roll our own instead of _moveto/_lineto because LineTo typically
   omits the final pixel.
   ----------------------------------------------------------------------- */
static void draw_line(int x1, int y1, int x2, int y2, short color)
{
    int dx, dy, sx, sy, err, e2;

    _setcolor(color);
    dx = abs(x2 - x1);
    dy = abs(y2 - y1);
    sx = (x1 < x2) ? 1 : -1;
    sy = (y1 < y2) ? 1 : -1;
    err = dx - dy;

    for (;;) {
        _setpixel(x1, y1);
        if (x1 == x2 && y1 == y2) break;
        e2 = 2 * err;
        if (e2 > -dy) { err -= dy; x1 += sx; }
        if (e2 <  dx) { err += dx; y1 += sy; }
    }
}

/* -----------------------------------------------------------------------
   Internal: parser helpers
   ----------------------------------------------------------------------- */
typedef struct {
    char    str[256];   /* uppercase copy of command string */
    int     pos;
    int     len;
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

/* Parse a number. Returns the value.
   *valid = 0 on parse error, *undefined = 1 if no number present. */
static double parse_number(parse_ctx *ctx, int *valid, int *undefined)
{
    int    negative = 0;
    int    has_digit = 0;
    double int_part = 0.0;
    double frac_part = 0.0;
    double frac_div = 1.0;
    char   c;

    *valid = 1;
    *undefined = 0;

    /* skip whitespace before number */
    while (ctx->pos < ctx->len &&
           (ctx->str[ctx->pos] == ' ' || ctx->str[ctx->pos] == '\t'))
        ctx->pos++;

    if (ctx->pos >= ctx->len) {
        *undefined = 1;
        return 0.0;
    }

    c = ctx->str[ctx->pos];

    /* sign */
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

    /* fractional part */
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
        if (negative) {
            ctx->pos--;   /* back up past the lone '-' */
            *undefined = 1;
        } else {
            *undefined = 1;
        }
        return 0.0;
    }

    /* skip trailing whitespace */
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
   ----------------------------------------------------------------------- */
void draw(const char *cmd)
{
    parse_ctx ctx;
    int       i, cmdlen;

    /* direction vectors */
    double vx, vy;       /* Up */
    double hx, hy;       /* Right */
    double ex, ey;       /* Up-Right diagonal */
    double fx, fy;       /* Down-Right diagonal */

    double px, py;       /* current position */
    double px2, py2;     /* target position */
    double xx, yy;       /* movement delta */
    double d;
    double sin_ta, cos_ta;
    double ir;           /* aspect ratio (1:1) */
    short  col;
    int    prefix_b, prefix_n;
    int    num_valid, num_undef;

    /* copy and uppercase the command string */
    cmdlen = (int)strlen(cmd);
    if (cmdlen == 0) return;
    if (cmdlen > 255) cmdlen = 255;
    for (i = 0; i < cmdlen; i++)
        ctx.str[i] = (char)toupper((unsigned char)cmd[i]);
    ctx.str[cmdlen] = '\0';
    ctx.pos = 0;
    ctx.len = cmdlen;

    /* initialize position if first call */
    if (!draw_initialized) {
        struct videoconfig vc;
        _getvideoconfig(&vc);
        draw_pos_x = vc.numxpixels / 2;
        draw_pos_y = vc.numypixels / 2;
        draw_initialized = 1;
    }

    px  = draw_pos_x;
    py  = draw_pos_y;
    col = draw_color;
    ir  = 1.0;

    prefix_b = 0;
    prefix_n = 0;

    /* --- macro to apply rotation to direction vectors --- */
    #define APPLY_ROTATION() do { \
        vx = 0;   vy = -1;       \
        ex = ir;  ey = -1;       \
        hx = ir;  hy =  0;       \
        fx = ir;  fy =  1;       \
        if (draw_ta != 0.0) {    \
            double rad = draw_ta * (M_PI / 180.0); \
            double st = sin(rad), ct = cos(rad);    \
            double tx, ty;       \
            tx = vx; ty = vy; vx = tx*ct + ty*st; vy = ty*ct - tx*st; \
            tx = hx; ty = hy; hx = tx*ct + ty*st; hy = ty*ct - tx*st; \
            tx = ex; ty = ey; ex = tx*ct + ty*st; ey = ty*ct - tx*st; \
            tx = fx; ty = fy; fx = tx*ct + ty*st; fy = ty*ct - tx*st; \
        } \
    } while(0)

    APPLY_ROTATION();

    /* --- macro for direction movement (U/D/L/R/E/F/G/H) --- */
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

    /* --- main parse loop --- */
    while (ctx.pos < ctx.len) {
        char c = ctx.str[ctx.pos];

        /* skip whitespace and semicolons */
        if (c == ' ' || c == '\t' || c == ';') {
            ctx.pos++;
            continue;
        }

        switch (c) {

        /* direction commands */
        case 'U': xx =  vx; yy =  vy; DO_MOVEMENT(); break;
        case 'D': xx = -vx; yy = -vy; DO_MOVEMENT(); break;
        case 'L': xx = -hx; yy = -hy; DO_MOVEMENT(); break;
        case 'R': xx =  hx; yy =  hy; DO_MOVEMENT(); break;
        case 'E': xx =  ex; yy =  ey; DO_MOVEMENT(); break;
        case 'F': xx =  fx; yy =  fy; DO_MOVEMENT(); break;
        case 'G': xx = -ex; yy = -ey; DO_MOVEMENT(); break;
        case 'H': xx = -fx; yy = -fy; DO_MOVEMENT(); break;

        /* M - Move to absolute or relative position */
        case 'M': {
            int is_relative;
            ctx.pos++;
            /* skip whitespace after M */
            while (ctx.pos < ctx.len &&
                   (ctx.str[ctx.pos] == ' ' || ctx.str[ctx.pos] == '\t'))
                ctx.pos++;
            if (ctx.pos >= ctx.len) goto done;

            is_relative = (ctx.str[ctx.pos] == '+' || ctx.str[ctx.pos] == '-');

            px2 = parse_number(&ctx, &num_valid, &num_undef);
            if (!num_valid || num_undef) goto done;

            if (ctx.pos >= ctx.len || ctx.str[ctx.pos] != ',') goto done;
            ctx.pos++;

            py2 = parse_number(&ctx, &num_valid, &num_undef);
            if (!num_valid || num_undef) goto done;

            if (is_relative) {
                xx = (px2 * ir) * hx - (py2 * ir) * vx;
                yy = px2 * hy - py2 * vy;
                px2 = px + xx * draw_scale;
                py2 = py + yy * draw_scale;
            }

            if (!prefix_b)
                draw_line(qb_round(px), qb_round(py),
                          qb_round(px2), qb_round(py2), col);
            if (!prefix_n) { px = px2; py = py2; }
            prefix_b = 0;
            prefix_n = 0;
            break;
        }

        /* B - Move without drawing prefix */
        case 'B':
            prefix_b = 1;
            ctx.pos++;
            break;

        /* N - Draw without moving prefix */
        case 'N':
            prefix_n = 1;
            ctx.pos++;
            break;

        /* C - Set color */
        case 'C':
            ctx.pos++;
            d = parse_number(&ctx, &num_valid, &num_undef);
            if (!num_valid || num_undef) goto done;
            col = (short)d;
            draw_color = col;
            break;

        /* S - Set scale */
        case 'S':
            ctx.pos++;
            d = parse_number(&ctx, &num_valid, &num_undef);
            if (!num_valid || num_undef) goto done;
            if (d < 0) goto done;
            draw_scale = d / 4.0;
            break;

        /* A - Set angle (0-3 = 0/90/180/270 degrees) */
        case 'A':
            ctx.pos++;
            d = parse_number(&ctx, &num_valid, &num_undef);
            if (!num_valid || num_undef) goto done;
            switch ((int)d) {
                case 0: draw_ta = 0;   break;
                case 1: draw_ta = 90;  break;
                case 2: draw_ta = 180; break;
                case 3: draw_ta = 270; break;
                default: goto done;
            }
            APPLY_ROTATION();
            break;

        /* T - must be TA (Turn Angle) */
        case 'T':
            ctx.pos++;
            while (ctx.pos < ctx.len &&
                   (ctx.str[ctx.pos] == ' ' || ctx.str[ctx.pos] == '\t'))
                ctx.pos++;
            if (ctx.pos >= ctx.len) goto done;
            if (ctx.str[ctx.pos] != 'A') goto done;
            ctx.pos++;
            d = parse_number(&ctx, &num_valid, &num_undef);
            if (!num_valid || num_undef) goto done;
            draw_ta = d;
            APPLY_ROTATION();
            break;

        /* P - Paint (flood fill) */
        case 'P': {
            short fill_col, border_col;
            ctx.pos++;
            d = parse_number(&ctx, &num_valid, &num_undef);
            if (!num_valid || num_undef) goto done;
            fill_col = (short)d;

            if (ctx.pos >= ctx.len || ctx.str[ctx.pos] != ',') goto done;
            ctx.pos++;

            d = parse_number(&ctx, &num_valid, &num_undef);
            if (!num_valid || num_undef) goto done;
            border_col = (short)d;

            _setcolor(fill_col);
            _floodfill(qb_round(px), qb_round(py), border_col);
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
    draw_initialized = 1;
}
