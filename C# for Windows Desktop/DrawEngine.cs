// DrawEngine.cs
// ============================================================================
// QBASIC/QB64PE-compatible DRAW command engine for C# / WinForms
// Ported from grxdraw.c (DJGPP + GRX) by RetroNick2020
//
// grxdraw.c  ->  DrawEngine.cs substitutions:
//   GrPlot(x,y,color)              -> SetPixel(bmp, x, y, color)
//   GrFloodFill(x,y,border,fill)   -> FloodFill(bmp, x, y, border, fill)
//   GrSizeX() / GrSizeY()          -> _bitmap.Width / _bitmap.Height
//   GrColor (short/ulong)          -> int  (EGA palette index 0-15)
//   static globals                 -> private instance fields
// ============================================================================

using System;
using System.Collections.Generic;
using System.Drawing;
using System.Drawing.Imaging;
using System.Runtime.InteropServices;

namespace DrawDemo
{
    /// <summary>
    /// Executes QBASIC DRAW command strings onto a <see cref="System.Drawing.Bitmap"/>.
    /// Persistent state (position, color, scale, angle) survives across Execute() calls,
    /// matching the behaviour of the original C implementation.
    /// </summary>
    public class DrawEngine
    {
        // -------------------------------------------------------------------------
        // EGA / CGA 16-color palette  (indices 0-15)
        // -------------------------------------------------------------------------
        private static readonly Color[] EgaPalette = new Color[]
        {
            Color.FromArgb(0,   0,   0  ),  //  0 - Black
            Color.FromArgb(0,   0,   170),  //  1 - Dark Blue
            Color.FromArgb(0,   170, 0  ),  //  2 - Dark Green
            Color.FromArgb(0,   170, 170),  //  3 - Dark Cyan
            Color.FromArgb(170, 0,   0  ),  //  4 - Dark Red
            Color.FromArgb(170, 0,   170),  //  5 - Dark Magenta
            Color.FromArgb(170, 85,  0  ),  //  6 - Brown
            Color.FromArgb(170, 170, 170),  //  7 - Light Gray
            Color.FromArgb(85,  85,  85 ),  //  8 - Dark Gray
            Color.FromArgb(85,  85,  255),  //  9 - Blue
            Color.FromArgb(85,  255, 85 ),  // 10 - Green
            Color.FromArgb(85,  255, 255),  // 11 - Cyan
            Color.FromArgb(255, 85,  85 ),  // 12 - Red / Light Red
            Color.FromArgb(255, 85,  255),  // 13 - Magenta
            Color.FromArgb(255, 255, 85 ),  // 14 - Yellow
            Color.FromArgb(255, 255, 255),  // 15 - White
        };

        // -------------------------------------------------------------------------
        // Persistent state (survives across Execute() calls)
        // -------------------------------------------------------------------------
        private double _posX        = 0.0;
        private double _posY        = 0.0;
        private int    _colorIndex  = 15;      // white
        private double _scale       = 1.0;     // S4 = 1 pixel/unit
        private double _angleTA     = 0.0;     // degrees
        private bool   _initialized = false;

        private Bitmap _bitmap;

        // -------------------------------------------------------------------------
        // Construction
        // -------------------------------------------------------------------------
        public DrawEngine(Bitmap bitmap)
        {
            _bitmap = bitmap ?? throw new ArgumentNullException(nameof(bitmap));
        }

        /// <summary>Replace the backing bitmap (e.g. after a canvas resize or clear).</summary>
        public void SetBitmap(Bitmap bitmap)
        {
            _bitmap = bitmap ?? throw new ArgumentNullException(nameof(bitmap));
            _initialized = false;   // re-center on next Execute()
        }

        // -------------------------------------------------------------------------
        // Public accessors  (mirror the C API in grxdraw.h)
        // -------------------------------------------------------------------------
        public double PosX         { get => _posX;       set { _posX = value;       _initialized = true;  } }
        public double PosY         { get => _posY;       set { _posY = value;       _initialized = true;  } }
        public int    ColorIndex   { get => _colorIndex; set { _colorIndex = Clamp(value, 0, 15); } }
        public double Scale        { get => _scale;      set { _scale = value; } }
        public double AngleDegrees { get => _angleTA;    set { _angleTA = value; } }

        public void SetPos(double x, double y) { _posX = x; _posY = y; _initialized = true; }

        // -------------------------------------------------------------------------
        // Execute a DRAW command string
        // -------------------------------------------------------------------------
        public void Execute(string cmd)
        {
            if (string.IsNullOrEmpty(cmd)) return;

            // Uppercase the command string once
            string s = cmd.ToUpperInvariant();
            int pos = 0, len = s.Length;

            // Default starting position = centre of bitmap on first call
            if (!_initialized)
            {
                _posX = _bitmap.Width  / 2.0;
                _posY = _bitmap.Height / 2.0;
                _initialized = true;
            }

            double px = _posX, py = _posY;
            int    col = _colorIndex;
            double ir  = 1.0;         // 1:1 pixel aspect ratio

            bool prefixB = false;     // B: move without drawing
            bool prefixN = false;     // N: draw without moving

            // Direction vectors - rebuilt whenever the angle changes
            double vx, vy, hx, hy, ex, ey, fx, fy;
            BuildVectors(ir, out vx, out vy, out hx, out hy, out ex, out ey, out fx, out fy);

            // ------------------------------------------------------------------
            // Main parse loop
            // ------------------------------------------------------------------
            while (pos < len)
            {
                char c = s[pos];

                // Skip whitespace and semicolons between tokens
                if (c == ' ' || c == '\t' || c == ';') { pos++; continue; }

                double xx = 0, yy = 0;

                switch (c)
                {
                    // ---- directional movement ----
                    case 'U': xx =  vx; yy =  vy; DoMove(s, ref pos, ref px, ref py, xx, yy, ir, col, ref prefixB, ref prefixN); break;
                    case 'D': xx = -vx; yy = -vy; DoMove(s, ref pos, ref px, ref py, xx, yy, ir, col, ref prefixB, ref prefixN); break;
                    case 'L': xx = -hx; yy = -hy; DoMove(s, ref pos, ref px, ref py, xx, yy, ir, col, ref prefixB, ref prefixN); break;
                    case 'R': xx =  hx; yy =  hy; DoMove(s, ref pos, ref px, ref py, xx, yy, ir, col, ref prefixB, ref prefixN); break;
                    case 'E': xx =  ex; yy =  ey; DoMove(s, ref pos, ref px, ref py, xx, yy, ir, col, ref prefixB, ref prefixN); break;
                    case 'F': xx =  fx; yy =  fy; DoMove(s, ref pos, ref px, ref py, xx, yy, ir, col, ref prefixB, ref prefixN); break;
                    case 'G': xx = -ex; yy = -ey; DoMove(s, ref pos, ref px, ref py, xx, yy, ir, col, ref prefixB, ref prefixN); break;
                    case 'H': xx = -fx; yy = -fy; DoMove(s, ref pos, ref px, ref py, xx, yy, ir, col, ref prefixB, ref prefixN); break;

                    // ---- M: move to absolute or relative position ----
                    case 'M':
                    {
                        pos++;
                        SkipSpaces(s, ref pos, len);
                        if (pos >= len) goto done;

                        bool isRelative = (s[pos] == '+' || s[pos] == '-');

                        double px2 = ParseNumber(s, ref pos, len, out bool v1, out bool u1);
                        if (!v1 || u1) goto done;
                        if (pos >= len || s[pos] != ',') goto done;
                        pos++;
                        double py2 = ParseNumber(s, ref pos, len, out bool v2, out bool u2);
                        if (!v2 || u2) goto done;

                        if (isRelative)
                        {
                            double dxx = (px2 * ir) * hx - (py2 * ir) * vx;
                            double dyy =  px2 * hy  -  py2 * vy;
                            px2 = px + dxx * _scale;
                            py2 = py + dyy * _scale;
                        }

                        if (!prefixB) DrawLine(QBRound(px), QBRound(py), QBRound(px2), QBRound(py2), col);
                        if (!prefixN) { px = px2; py = py2; }
                        prefixB = false; prefixN = false;
                        break;
                    }

                    case 'B': prefixB = true;  pos++; break;
                    case 'N': prefixN = true;  pos++; break;

                    // ---- C: set color ----
                    case 'C':
                    {
                        pos++;
                        double d = ParseNumber(s, ref pos, len, out bool v, out bool u);
                        if (!v || u) goto done;
                        col = Clamp((int)d, 0, 15);
                        _colorIndex = col;
                        break;
                    }

                    // ---- S: set scale ----
                    case 'S':
                    {
                        pos++;
                        double d = ParseNumber(s, ref pos, len, out bool v, out bool u);
                        if (!v || u) goto done;
                        if (d < 0) goto done;
                        _scale = d / 4.0;
                        break;
                    }

                    // ---- A: set angle by index (0-3) ----
                    case 'A':
                    {
                        pos++;
                        double d = ParseNumber(s, ref pos, len, out bool v, out bool u);
                        if (!v || u) goto done;
                        switch ((int)d)
                        {
                            case 0: _angleTA =   0.0; break;
                            case 1: _angleTA =  90.0; break;
                            case 2: _angleTA = 180.0; break;
                            case 3: _angleTA = 270.0; break;
                            default: goto done;
                        }
                        BuildVectors(ir, out vx, out vy, out hx, out hy, out ex, out ey, out fx, out fy);
                        break;
                    }

                    // ---- T: must be TA (turn angle in degrees) ----
                    case 'T':
                    {
                        pos++;
                        SkipSpaces(s, ref pos, len);
                        if (pos >= len || s[pos] != 'A') goto done;
                        pos++;
                        double d = ParseNumber(s, ref pos, len, out bool v, out bool u);
                        if (!v || u) goto done;
                        _angleTA = d;
                        BuildVectors(ir, out vx, out vy, out hx, out hy, out ex, out ey, out fx, out fy);
                        break;
                    }

                    // ---- P: paint / flood fill ----
                    case 'P':
                    {
                        pos++;
                        double d1 = ParseNumber(s, ref pos, len, out bool v1, out bool u1);
                        if (!v1 || u1) goto done;
                        if (pos >= len || s[pos] != ',') goto done;
                        pos++;
                        double d2 = ParseNumber(s, ref pos, len, out bool v2, out bool u2);
                        if (!v2 || u2) goto done;

                        int fillCol   = Clamp((int)d1, 0, 15);
                        int borderCol = Clamp((int)d2, 0, 15);
                        FloodFill(QBRound(px), QBRound(py), EgaPalette[borderCol], EgaPalette[fillCol]);
                        col = fillCol;
                        _colorIndex = col;
                        break;
                    }

                    default: pos++; break;
                }
            }

            done:
            _posX = px;
            _posY = py;
            _initialized = true;
        }

        // -------------------------------------------------------------------------
        // Helper: execute one directional move (U/D/L/R/E/F/G/H)
        // -------------------------------------------------------------------------
        private void DoMove(string s, ref int pos, ref double px, ref double py,
                            double xx, double yy, double ir, int col,
                            ref bool prefixB, ref bool prefixN)
        {
            pos++;
            double d = ParseNumber(s, ref pos, s.Length, out bool valid, out bool undef);
            if (!valid) return;
            if (undef) d = 1.0;

            xx *= d * ir;
            yy *= d;

            double px2 = px + xx * _scale;
            double py2 = py + yy * _scale;

            if (!prefixB) DrawLine(QBRound(px), QBRound(py), QBRound(px2), QBRound(py2), col);
            if (!prefixN) { px = px2; py = py2; }

            prefixB = false;
            prefixN = false;
        }

        // -------------------------------------------------------------------------
        // Helper: (re)build direction vectors from current _angleTA
        // -------------------------------------------------------------------------
        private void BuildVectors(double ir,
                                  out double vx, out double vy,
                                  out double hx, out double hy,
                                  out double ex, out double ey,
                                  out double fx, out double fy)
        {
            vx = 0.0;  vy = -1.0;
            hx = ir;   hy =  0.0;
            ex = ir;   ey = -1.0;
            fx = ir;   fy =  1.0;

            if (_angleTA == 0.0) return;

            double rad = _angleTA * (Math.PI / 180.0);
            double st  = Math.Sin(rad);
            double ct  = Math.Cos(rad);

            Rotate(ref vx, ref vy, st, ct);
            Rotate(ref hx, ref hy, st, ct);
            Rotate(ref ex, ref ey, st, ct);
            Rotate(ref fx, ref fy, st, ct);
        }

        private static void Rotate(ref double x, ref double y, double sinA, double cosA)
        {
            double nx = x * cosA + y * sinA;
            double ny = y * cosA - x * sinA;
            x = nx; y = ny;
        }

        // -------------------------------------------------------------------------
        // Bresenham line  (mirrors the C draw_line function exactly)
        // Uses direct bitmap pixel access for performance.
        // -------------------------------------------------------------------------
        private void DrawLine(int x1, int y1, int x2, int y2, int colorIdx)
        {
            Color c = EgaPalette[Clamp(colorIdx, 0, 15)];

            int w = _bitmap.Width, h = _bitmap.Height;
            int dx = Math.Abs(x2 - x1);
            int dy = Math.Abs(y2 - y1);
            int sx = x1 < x2 ? 1 : -1;
            int sy = y1 < y2 ? 1 : -1;
            int err = dx - dy;

            // Lock the bitmap for fast pixel access
            BitmapData bmpData = _bitmap.LockBits(
                new Rectangle(0, 0, w, h),
                ImageLockMode.ReadWrite,
                PixelFormat.Format32bppArgb);

            unsafe
            {
                byte* ptr = (byte*)bmpData.Scan0;
                int   stride = bmpData.Stride;

                // Pre-compose the ARGB int once
                int argb = c.ToArgb();

                while (true)
                {
                    // Plot pixel if within bounds
                    if (x1 >= 0 && x1 < w && y1 >= 0 && y1 < h)
                    {
                        int* pixel = (int*)(ptr + y1 * stride + x1 * 4);
                        *pixel = argb;
                    }

                    if (x1 == x2 && y1 == y2) break;
                    int e2 = 2 * err;
                    if (e2 > -dy) { err -= dy; x1 += sx; }
                    if (e2 <  dx) { err += dx; y1 += sy; }
                }
            }

            _bitmap.UnlockBits(bmpData);
        }

        // -------------------------------------------------------------------------
        // Scanline flood fill  (replaces GrFloodFill / _floodfill)
        // Stack-based scanline algorithm; safe on 640x480-scale bitmaps.
        // -------------------------------------------------------------------------
        private void FloodFill(int startX, int startY, Color borderColor, Color fillColor)
        {
            int w = _bitmap.Width, h = _bitmap.Height;
            if (startX < 0 || startY < 0 || startX >= w || startY >= h) return;

            int borderArgb = borderColor.ToArgb();
            int fillArgb   = fillColor.ToArgb();

            // Lock bitmap for the entire fill operation
            BitmapData bmpData = _bitmap.LockBits(
                new Rectangle(0, 0, w, h),
                ImageLockMode.ReadWrite,
                PixelFormat.Format32bppArgb);

            unsafe
            {
                byte* scan0  = (byte*)bmpData.Scan0;
                int   stride = bmpData.Stride;

                int* GetPixelPtr(int x, int y) =>
                    (int*)(scan0 + y * stride + x * 4);

                int startArgb = *GetPixelPtr(startX, startY);
                // Nothing to do if the seed is already the fill color or the border
                if (startArgb == fillArgb || startArgb == borderArgb)
                {
                    _bitmap.UnlockBits(bmpData);
                    return;
                }

                // Stack of (x, y) seed points
                var stack = new Stack<(int x, int y)>(4096);
                stack.Push((startX, startY));

                while (stack.Count > 0)
                {
                    var (cx, cy) = stack.Pop();

                    if (cx < 0 || cx >= w || cy < 0 || cy >= h) continue;
                    int pxVal = *GetPixelPtr(cx, cy);
                    if (pxVal == borderArgb || pxVal == fillArgb) continue;

                    // Scan left to find the left edge of this span
                    int lx = cx;
                    while (lx > 0)
                    {
                        int left = *GetPixelPtr(lx - 1, cy);
                        if (left == borderArgb || left == fillArgb) break;
                        lx--;
                    }

                    // Fill rightward and push adjacent rows
                    int rx = lx;
                    while (rx < w)
                    {
                        int cur = *GetPixelPtr(rx, cy);
                        if (cur == borderArgb || cur == fillArgb) break;

                        *GetPixelPtr(rx, cy) = fillArgb;

                        if (cy > 0)
                        {
                            int above = *GetPixelPtr(rx, cy - 1);
                            if (above != borderArgb && above != fillArgb)
                                stack.Push((rx, cy - 1));
                        }
                        if (cy < h - 1)
                        {
                            int below = *GetPixelPtr(rx, cy + 1);
                            if (below != borderArgb && below != fillArgb)
                                stack.Push((rx, cy + 1));
                        }
                        rx++;
                    }
                }
            }

            _bitmap.UnlockBits(bmpData);
        }

        // -------------------------------------------------------------------------
        // Number parser  (port of parse_number from grxdraw.c)
        // -------------------------------------------------------------------------
        private static double ParseNumber(string s, ref int pos, int len,
                                          out bool valid, out bool undefined)
        {
            valid     = true;
            undefined = false;

            // Skip leading spaces/tabs
            while (pos < len && (s[pos] == ' ' || s[pos] == '\t')) pos++;

            if (pos >= len) { undefined = true; return 0.0; }

            bool   negative = false;
            bool   hasDigit = false;
            double intPart  = 0.0;
            double fracPart = 0.0;
            double fracDiv  = 1.0;

            char c = s[pos];
            if (c == '-')      { negative = true; pos++; }
            else if (c == '+') { pos++; }

            while (pos < len && s[pos] >= '0' && s[pos] <= '9')
            {
                intPart = intPart * 10.0 + (s[pos] - '0');
                hasDigit = true;
                pos++;
            }

            if (pos < len && s[pos] == '.')
            {
                pos++;
                while (pos < len && s[pos] >= '0' && s[pos] <= '9')
                {
                    fracPart = fracPart * 10.0 + (s[pos] - '0');
                    fracDiv *= 10.0;
                    hasDigit = true;
                    pos++;
                }
            }

            if (!hasDigit)
            {
                if (negative) pos--;   // back up past lone '-'
                undefined = true;
                return 0.0;
            }

            // Skip trailing spaces/tabs
            while (pos < len && (s[pos] == ' ' || s[pos] == '\t')) pos++;

            double result = intPart + fracPart / fracDiv;
            return negative ? -result : result;
        }

        // -------------------------------------------------------------------------
        // QB-compatible round  (Trunc(x + 0.5) for positive, Trunc(x - 0.5) for neg)
        // -------------------------------------------------------------------------
        private static int QBRound(double d)
            => d >= 0.0 ? (int)(d + 0.5) : (int)(d - 0.5);

        private static void SkipSpaces(string s, ref int pos, int len)
        {
            while (pos < len && (s[pos] == ' ' || s[pos] == '\t')) pos++;
        }

        private static int Clamp(int v, int lo, int hi)
            => v < lo ? lo : v > hi ? hi : v;
    }
}
