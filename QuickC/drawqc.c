/*
   drawqc.c  -  QBASIC/QB64PE compatible DRAW command
                   Microsoft QuickC 2.1 port
   =====================================================
   Released April 2026 by RetroNick2020
   Ported to Microsoft QuickC 2.1 (Microsoft graph.h).

   Compile with:
     qcl /AL testqc.c drawqc.c graphics.lib
   
   Please visit https://retronick2020.itch.io/vecdraw-v10-for-qbasicgwbasicqb64pe
   Download VecDraw to draw fancy graphics/text and import SVG's to create
   stunning graphics.  Everything exports to Draw statements.  VecDraw is FREE.

   Visit https://retronick2020.itch.io for other great programming utilities.

   PORTING NOTES (OpenWatcom -> QuickC 2.1):
     QuickC 2.1 uses Microsoft's <graph.h> which is very close to OpenWatcom's.
     Key differences:
       - _setpixel() returns a short in QC (we capture and discard it)
       - _getch() is preferred over getch() in QC (both usually work)
       - Compile switch is /AL (large model) instead of -ml
       - _VRES16COLOR may not be available on all adapters; _ERESCOLOR
         (EGA 640x350x16) is a safe fallback defined below
       - far pointer keyword 'far' may be needed for large model string
         args - we use 'const char *' which QC handles correctly in /AL
*/

#include <graph.h>
#include <math.h>
#include <string.h>
#include <ctype.h>
#include "drawqc.h"

#ifndef M_PI
#define M_PI 3.14159265358979323846
#endif

/*
   QuickC 2.1 may not define _VRES16COLOR (VGA 640x480x16).
   _ERESCOLOR (EGA 640x350x16) is the safest high-res 16-colour mode.
   drawtest_qc.c uses _ERESCOLOR so it works on EGA and VGA alike.
   If you have a VGA adapter you can change it to _VRES16COLOR there.
*/

/* -----------------------------------------------------------------------
   Persistent state across draw() calls
   ----------------------------------------------------------------------- */
static double draw_pos_x    = 0.0;
static double draw_pos_y    = 0.0;
static short  draw_color    = 15;
static double draw_scale    = 1.0;   /* S/4 - default S4 = 1.0 */
static double draw_ta       = 0.0;   /* turn angle in degrees */
static int    draw_initialized = 0;

/* -----------------------------------------------------------------------
   Accessors
   ----------------------------------------------------------------------- */
double draw_get_x(void)               { return draw_pos_x; }
double draw_get_y(void)               { return draw_pos_y; }
void   draw_set_pos(double ax, double ay) {
    draw_pos_x       = ax;
    draw_pos_y       = ay;
    draw_initialized = 1;
}
short  draw_get_color(void)           { return draw_color; }
void   draw_set_color(short color)    { draw_color = color; }
double draw_get_scale(void)           { return draw_scale; }
void   draw_set_scale(double scale)   { draw_scale = scale; }
double draw_get_angle(void)           { return draw_ta; }
void   draw_set_angle(double deg)     { draw_ta = deg; }

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
   Internal: Bresenham line using _setpixel.
   In QuickC 2.1, _setpixel(short x, short y) returns short (old pixel
   colour).  We cast coords to short to keep the compiler happy.
   ----------------------------------------------------------------------- */
static void draw_line(int x1, int y1, int x2, int y2, short color)
{
    int dx, dy, sx, sy, err, e2;

    _setcolor(color);

    dx  = abs(x2 - x1);
    dy  = abs(y2 - y1);
    sx  = (x1 < x2) ? 1 : -1;
    sy  = (y1 < y2) ? 1 : -1;
    err = dx - dy;

    for (;;) {
        _setpixel((short)x1, (short)y1);  /* QC: _setpixel takes short */
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
    char str[256];
    int  pos;
    int  len;
} parse_ctx;

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
            has_digit = 1;
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
   ----------------------------------------------------------------------- */
void draw(const char *cmd)
{
    parse_ctx ctx;
    int       i, cmdlen;

    double vx, vy;
    double hx, hy;
    double ex, ey;
    double fx, fy;

    double px, py;
    double px2, py2;
    double xx, yy;
    double d;
    double ir;
    short  col;
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

    /* initialize position on first call */
    if (!draw_initialized) {
        struct videoconfig vc;
        _getvideoconfig(&vc);              /* QC: same as OpenWatcom */
        draw_pos_x       = vc.numxpixels / 2;
        draw_pos_y       = vc.numypixels / 2;
        draw_initialized = 1;
    }

    px  = draw_pos_x;
    py  = draw_pos_y;
    col = draw_color;
    ir  = 1.0;

    prefix_b = 0;
    prefix_n = 0;

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

    while (ctx.pos < ctx.len) {
        char c = ctx.str[ctx.pos];

        if (c == ' ' || c == '\t' || c == ';') {
            ctx.pos++;
            continue;
        }

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

            is_relative = (ctx.str[ctx.pos] == '+' || ctx.str[ctx.pos] == '-');

            px2 = parse_number(&ctx, &num_valid, &num_undef);
            if (!num_valid || num_undef) goto done;

            if (ctx.pos >= ctx.len || ctx.str[ctx.pos] != ',') goto done;
            ctx.pos++;

            py2 = parse_number(&ctx, &num_valid, &num_undef);
            if (!num_valid || num_undef) goto done;

            if (is_relative) {
                xx  = (px2 * ir) * hx - (py2 * ir) * vx;
                yy  =  px2 * hy - py2 * vy;
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

        case 'B':
            prefix_b = 1;
            ctx.pos++;
            break;

        case 'N':
            prefix_n = 1;
            ctx.pos++;
            break;

        case 'C':
            ctx.pos++;
            d = parse_number(&ctx, &num_valid, &num_undef);
            if (!num_valid || num_undef) goto done;
            col = (short)d;
            draw_color = col;
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

        /* P - Paint (flood fill).
           QC 2.1: _floodfill(x, y, border_color) fills with the current
           color set by _setcolor().  Same API as OpenWatcom. */
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
            _floodfill((short)qb_round(px), (short)qb_round(py), border_col);
            col = fill_col;
            draw_color = col;
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

    draw_pos_x   = px;
    draw_pos_y   = py;
    draw_initialized = 1;
}
