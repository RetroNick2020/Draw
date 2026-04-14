/*
   owdraw.js  -  QBASIC/QB64PE compatible DRAW command
                 JavaScript / HTML5 Canvas port
   ====================================================
   Released April 2026 by RetroNick2020

   Please visit https://retronick2020.itch.io/vecdraw-v10-for-qbasicgwbasicqb64pe
   Download VecDraw to draw fancy graphics/text and import SVG's to create
   stunning graphics.  Everything exports to Draw statements.  VecDraw is FREE.

   Visit https://retronick2020.itch.io for other great programming utilities.

   USAGE
   -----
   1. Add a <canvas> element to your HTML.
   2. Call Draw.init(canvasElement)  -  once, after the DOM is ready.
   3. Call Draw.draw(string)         -  as many times as you like.
   4. Call Draw.clear()              -  to wipe the canvas.

   QUICK EXAMPLE
   -------------
     const canvas = document.getElementById('myCanvas');
     Draw.init(canvas);
     Draw.setPos(160, 100);
     Draw.draw('C14 S8 R50 D50 L50 U50');   // yellow square

   DRAW COMMAND REFERENCE
   ----------------------
   U/D/L/R/E/F/G/H [n]   Move/draw in 8 directions n pixels (default 1)
   M [+|-]x,y             Move/draw to absolute (or relative) coords
   B                      Prefix: blind move (no line drawn)
   N                      Prefix: no advance (position unchanged after draw)
   C n                    Set colour (0-15, QB CGA/EGA palette index)
   S n                    Set scale factor (S4 = 1:1 default)
   A n                    Angle preset (0=0°, 1=90°, 2=180°, 3=270°)
   TA n                   Turn angle in degrees (-360 to 360)
   P fill,border          Flood fill at current position

   PORTING NOTES (OpenWatcom C -> JavaScript)
   ------------------------------------------
   _setpixel(x,y)         -> setPixel(x, y, color)  using ImageData
   _setcolor(c)           -> QB palette index -> rgba string
   _floodfill(x,y,b)      -> custom stack flood fill on Uint32Array shadow buf
   _getvideoconfig()      -> canvas.width / canvas.height
   short color            -> QB palette index 0-15 -> CSS rgba string

   FLOOD FILL
   ----------
   A Uint32Array shadow buffer (one ABGR uint32 per pixel, matching
   ImageData layout) mirrors every pixel painted.  The P command runs a
   stack-based 4-connected fill against this buffer so no getImageData()
   readback is needed mid-draw.  At the end of draw() (or on demand via
   Draw.flush()) the whole ImageData is written to the canvas in one call.

   RENDERING STRATEGY
   ------------------
   All pixel writes go to an off-screen ImageData object.  draw() commits
   it to the visible canvas automatically after each call.  Call
   Draw.beginBatch() before a series of draw() calls and Draw.endBatch()
   after to defer the commit and improve performance when issuing many
   commands in quick succession.
*/

const Draw = (function () {
    'use strict';

    /* ------------------------------------------------------------------
       QB/CGA 16-colour palette.
       Stored as packed ABGR Uint32 values to match ImageData byte order
       (ImageData is RGBA in memory order = ABGR when read as Uint32 on
       little-endian, which all modern x86/ARM browsers use).
       ------------------------------------------------------------------ */
    const QB_PALETTE_RGBA = [
        /* idx  name           r    g    b */
        [   0,   0,   0, 255],  /*  0  Black        */
        [   0,   0, 170, 255],  /*  1  Dark Blue     */
        [   0, 170,   0, 255],  /*  2  Dark Green    */
        [   0, 170, 170, 255],  /*  3  Dark Cyan     */
        [ 170,   0,   0, 255],  /*  4  Dark Red      */
        [ 170,   0, 170, 255],  /*  5  Dark Magenta  */
        [ 170,  85,   0, 255],  /*  6  Brown         */
        [ 170, 170, 170, 255],  /*  7  Light Gray    */
        [  85,  85,  85, 255],  /*  8  Dark Gray     */
        [  85,  85, 255, 255],  /*  9  Blue          */
        [  85, 255,  85, 255],  /* 10  Green         */
        [  85, 255, 255, 255],  /* 11  Cyan          */
        [ 255,  85,  85, 255],  /* 12  Red           */
        [ 255,  85, 255, 255],  /* 13  Magenta       */
        [ 255, 255,  85, 255],  /* 14  Yellow        */
        [ 255, 255, 255, 255],  /* 15  White         */
    ];

    /* Pack RGBA array into Uint32 (little-endian ABGR) */
    function packColor(rgba) {
        return ((rgba[3] << 24) | (rgba[2] << 16) | (rgba[1] << 8) | rgba[0]) >>> 0;
    }

    /* Pre-packed palette for fast comparison in flood fill */
    const PALETTE_U32 = QB_PALETTE_RGBA.map(packColor);

    /* CSS rgba() strings for ctx.fillStyle (used nowhere here, kept for
       external callers who want to use Draw.qbColor(n) directly) */
    const PALETTE_CSS = QB_PALETTE_RGBA.map(
        c => `rgba(${c[0]},${c[1]},${c[2]},${c[3] / 255})`
    );

    /* ------------------------------------------------------------------
       Internal state
       ------------------------------------------------------------------ */
    let _canvas      = null;
    let _ctx         = null;
    let _imgData     = null;   /* ImageData object          */
    let _buf32       = null;   /* Uint32Array view of above */
    let _w           = 0;
    let _h           = 0;
    let _batchMode   = false;  /* true = defer canvas flush */

    let _posX        = 0.0;
    let _posY        = 0.0;
    let _colorIdx    = 15;     /* QB colour index 0-15      */
    let _scale       = 1.0;    /* S/4; S4 = 1.0             */
    let _ta          = 0.0;    /* turn angle degrees        */
    let _initialized = false;

    /* ------------------------------------------------------------------
       Lifecycle
       ------------------------------------------------------------------ */
    function init(canvasElement) {
        _canvas = canvasElement;
        _ctx    = canvasElement.getContext('2d');
        _w      = canvasElement.width;
        _h      = canvasElement.height;

        _imgData = _ctx.createImageData(_w, _h);
        _buf32   = new Uint32Array(_imgData.data.buffer);

        /* Clear to black */
        _buf32.fill(PALETTE_U32[0]);

        _posX        = _w / 2;
        _posY        = _h / 2;
        _initialized = true;
        _flush();
    }

    function clear(colorIdx) {
        const ci = (colorIdx !== undefined) ? (colorIdx & 15) : 0;
        _buf32.fill(PALETTE_U32[ci]);
        _flush();
    }

    /* Defer canvas commits until endBatch() for bulk draw() calls */
    function beginBatch() { _batchMode = true; }
    function endBatch()   { _batchMode = false; _flush(); }

    function _flush() {
        if (_batchMode) return;
        _ctx.putImageData(_imgData, 0, 0);
    }

    /* ------------------------------------------------------------------
       State accessors
       ------------------------------------------------------------------ */
    function getX()              { return _posX; }
    function getY()              { return _posY; }
    function setPos(x, y)        { _posX = x; _posY = y; }
    function getColor()          { return _colorIdx; }
    function setColor(idx)       { _colorIdx = idx & 15; }
    function getScale()          { return _scale; }
    function setScale(s)         { _scale = s; }
    function getAngle()          { return _ta; }
    function setAngle(deg)       { _ta = deg; }
    function qbColor(idx)        { return PALETTE_CSS[idx & 15]; }

    /* ------------------------------------------------------------------
       Pixel & line helpers
       ------------------------------------------------------------------ */
    function qbRound(d) {
        return (d >= 0) ? Math.floor(d + 0.5) : Math.ceil(d - 0.5);
    }

    function _setPixel(x, y, u32) {
        if (x < 0 || x >= _w || y < 0 || y >= _h) return;
        _buf32[(y * _w + x) | 0] = u32;
    }

    /* Bresenham line - writes to Uint32Array shadow buffer */
    function _drawLine(x1, y1, x2, y2, u32) {
        x1 = x1 | 0; y1 = y1 | 0; x2 = x2 | 0; y2 = y2 | 0;
        let dx  = Math.abs(x2 - x1);
        let dy  = Math.abs(y2 - y1);
        let sx  = (x1 < x2) ? 1 : -1;
        let sy  = (y1 < y2) ? 1 : -1;
        let err = dx - dy;

        for (;;) {
            _setPixel(x1, y1, u32);
            if (x1 === x2 && y1 === y2) break;
            const e2 = 2 * err;
            if (e2 > -dy) { err -= dy; x1 += sx; }
            if (e2 <  dx) { err += dx; y1 += sy; }
        }
    }

    /* ------------------------------------------------------------------
       Stack-based 4-connected flood fill on the Uint32Array shadow buffer.

       Works entirely in RAM - no getImageData() readback needed.
       Uses a typed Int32Array stack for performance (avoids GC pressure
       from a JS array of objects).  Each stack entry is two Int32s: x, y.
       ------------------------------------------------------------------ */
    function _floodFill(sx, sy, fillU32, borderU32) {
        sx = sx | 0; sy = sy | 0;
        if (sx < 0 || sx >= _w || sy < 0 || sy >= _h) return;

        const target = _buf32[sy * _w + sx];
        if (target === borderU32) return;
        if (target === fillU32)   return;

        /* Pre-allocate a typed stack: pairs of (x, y) as Int32 */
        let stackCap  = 16384;          /* entries (each = 2 Int32 words)  */
        let stackData = new Int32Array(stackCap * 2);
        let stackTop  = 0;

        function push(x, y) {
            if (stackTop >= stackCap) {
                stackCap  *= 2;
                const ns  = new Int32Array(stackCap * 2);
                ns.set(stackData);
                stackData = ns;
            }
            stackData[stackTop * 2]     = x;
            stackData[stackTop * 2 + 1] = y;
            stackTop++;
        }

        push(sx, sy);

        while (stackTop > 0) {
            stackTop--;
            const x = stackData[stackTop * 2];
            const y = stackData[stackTop * 2 + 1];

            if (x < 0 || x >= _w || y < 0 || y >= _h) continue;

            const cur = _buf32[y * _w + x];
            if (cur === borderU32 || cur === fillU32) continue;

            _buf32[y * _w + x] = fillU32;

            push(x + 1, y);
            push(x - 1, y);
            push(x, y + 1);
            push(x, y - 1);
        }
    }

    /* ------------------------------------------------------------------
       Number parser  (mirrors the C implementation)
       Returns { value, valid, undefined }
       ------------------------------------------------------------------ */
    function _parseNumber(ctx) {
        let pos      = ctx.pos;
        const s      = ctx.str;
        const len    = ctx.len;
        let negative = false;
        let hasDigit = false;
        let intPart  = 0;
        let fracPart = 0;
        let fracDiv  = 1;

        /* skip whitespace */
        while (pos < len && (s[pos] === ' ' || s[pos] === '\t')) pos++;

        if (pos >= len) { ctx.pos = pos; return { value: 0, valid: true, undef: true }; }

        if      (s[pos] === '-') { negative = true;  pos++; }
        else if (s[pos] === '+') {                   pos++; }

        while (pos < len && s[pos] >= '0' && s[pos] <= '9') {
            intPart = intPart * 10 + (s.charCodeAt(pos) - 48);
            hasDigit = true;
            pos++;
        }

        if (pos < len && s[pos] === '.') {
            pos++;
            while (pos < len && s[pos] >= '0' && s[pos] <= '9') {
                fracPart = fracPart * 10 + (s.charCodeAt(pos) - 48);
                fracDiv *= 10;
                hasDigit  = true;
                pos++;
            }
        }

        if (!hasDigit) {
            if (negative) pos--;   /* back up past lone '-' */
            ctx.pos = pos;
            return { value: 0, valid: true, undef: true };
        }

        /* skip trailing whitespace */
        while (pos < len && (s[pos] === ' ' || s[pos] === '\t')) pos++;

        ctx.pos = pos;
        const result = intPart + fracPart / fracDiv;
        return { value: negative ? -result : result, valid: true, undef: false };
    }

    /* ------------------------------------------------------------------
       Main DRAW command parser
       ------------------------------------------------------------------ */
    function draw(cmd) {
        if (!_initialized) {
            console.warn('Draw.draw() called before Draw.init()');
            return;
        }
        if (!cmd || cmd.length === 0) return;

        const ctx = {
            str: cmd.toUpperCase().substring(0, 255),
            pos: 0,
            len: 0
        };
        ctx.len = ctx.str.length;

        let px  = _posX;
        let py  = _posY;
        let col = PALETTE_U32[_colorIdx];
        const ir = 1.0;   /* aspect ratio kept 1:1 */

        let prefixB = false;
        let prefixN = false;

        /* --- direction vectors (recomputed on A/TA commands) --- */
        let vx, vy, hx, hy, ex, ey, fx, fy;

        function applyRotation() {
            vx = 0;   vy = -1;
            ex = ir;  ey = -1;
            hx = ir;  hy =  0;
            fx = ir;  fy =  1;
            if (_ta !== 0.0) {
                const rad = _ta * (Math.PI / 180);
                const st  = Math.sin(rad);
                const ct  = Math.cos(rad);
                let tx, ty;
                tx=vx; ty=vy; vx=tx*ct+ty*st; vy=ty*ct-tx*st;
                tx=hx; ty=hy; hx=tx*ct+ty*st; hy=ty*ct-tx*st;
                tx=ex; ty=ey; ex=tx*ct+ty*st; ey=ty*ct-tx*st;
                tx=fx; ty=fy; fx=tx*ct+ty*st; fy=ty*ct-tx*st;
            }
        }

        applyRotation();

        /* --- direction movement helper --- */
        function doMovement(ddx, ddy) {
            ctx.pos++;
            const r = _parseNumber(ctx);
            if (!r.valid) return false;
            const d  = r.undef ? 1.0 : r.value;
            let xx   = ddx * d * ir;
            let yy   = ddy * d;
            const px2 = px + xx * _scale;
            const py2 = py + yy * _scale;
            if (!prefixB)
                _drawLine(qbRound(px), qbRound(py), qbRound(px2), qbRound(py2), col);
            if (!prefixN) { px = px2; py = py2; }
            prefixB = false;
            prefixN = false;
            return true;
        }

        /* --- main loop --- */
        outer:
        while (ctx.pos < ctx.len) {
            const c = ctx.str[ctx.pos];

            if (c === ' ' || c === '\t' || c === ';') { ctx.pos++; continue; }

            switch (c) {
            case 'U': if (!doMovement( vx,  vy)) break outer; break;
            case 'D': if (!doMovement(-vx, -vy)) break outer; break;
            case 'L': if (!doMovement(-hx, -hy)) break outer; break;
            case 'R': if (!doMovement( hx,  hy)) break outer; break;
            case 'E': if (!doMovement( ex,  ey)) break outer; break;
            case 'F': if (!doMovement( fx,  fy)) break outer; break;
            case 'G': if (!doMovement(-ex, -ey)) break outer; break;
            case 'H': if (!doMovement(-fx, -fy)) break outer; break;

            case 'M': {
                ctx.pos++;
                while (ctx.pos < ctx.len &&
                       (ctx.str[ctx.pos] === ' ' || ctx.str[ctx.pos] === '\t'))
                    ctx.pos++;
                if (ctx.pos >= ctx.len) break outer;

                const isRelative = (ctx.str[ctx.pos] === '+' ||
                                    ctx.str[ctx.pos] === '-');

                const rx = _parseNumber(ctx);
                if (!rx.valid || rx.undef) break outer;

                if (ctx.pos >= ctx.len || ctx.str[ctx.pos] !== ',') break outer;
                ctx.pos++;

                const ry = _parseNumber(ctx);
                if (!ry.valid || ry.undef) break outer;

                let px2 = rx.value;
                let py2 = ry.value;

                if (isRelative) {
                    const xx = (px2 * ir) * hx - (py2 * ir) * vx;
                    const yy =  px2 * hy  - py2 * vy;
                    px2 = px + xx * _scale;
                    py2 = py + yy * _scale;
                }

                if (!prefixB)
                    _drawLine(qbRound(px), qbRound(py),
                              qbRound(px2), qbRound(py2), col);
                if (!prefixN) { px = px2; py = py2; }
                prefixB = false;
                prefixN = false;
                break;
            }

            case 'B': prefixB = true; ctx.pos++; break;
            case 'N': prefixN = true; ctx.pos++; break;

            case 'C': {
                ctx.pos++;
                const r = _parseNumber(ctx);
                if (!r.valid || r.undef) break outer;
                _colorIdx = Math.floor(r.value) & 15;
                col = PALETTE_U32[_colorIdx];
                break;
            }

            case 'S': {
                ctx.pos++;
                const r = _parseNumber(ctx);
                if (!r.valid || r.undef) break outer;
                if (r.value < 0) break outer;
                _scale = r.value / 4.0;
                break;
            }

            case 'A': {
                ctx.pos++;
                const r = _parseNumber(ctx);
                if (!r.valid || r.undef) break outer;
                const map = [0, 90, 180, 270];
                const ai  = Math.floor(r.value);
                if (ai < 0 || ai > 3) break outer;
                _ta = map[ai];
                applyRotation();
                break;
            }

            case 'T': {
                ctx.pos++;
                while (ctx.pos < ctx.len &&
                       (ctx.str[ctx.pos] === ' ' || ctx.str[ctx.pos] === '\t'))
                    ctx.pos++;
                if (ctx.pos >= ctx.len)         break outer;
                if (ctx.str[ctx.pos] !== 'A')   break outer;
                ctx.pos++;
                const r = _parseNumber(ctx);
                if (!r.valid || r.undef) break outer;
                _ta = r.value;
                applyRotation();
                break;
            }

            /* P - Flood fill using shadow buffer (no getImageData readback) */
            case 'P': {
                ctx.pos++;
                const rf = _parseNumber(ctx);
                if (!rf.valid || rf.undef) break outer;

                if (ctx.pos >= ctx.len || ctx.str[ctx.pos] !== ',') break outer;
                ctx.pos++;

                const rb = _parseNumber(ctx);
                if (!rb.valid || rb.undef) break outer;

                const fillIdx   = Math.floor(rf.value) & 15;
                const borderIdx = Math.floor(rb.value) & 15;

                _floodFill(qbRound(px), qbRound(py),
                           PALETTE_U32[fillIdx],
                           PALETTE_U32[borderIdx]);

                _colorIdx = fillIdx;
                col = PALETTE_U32[_colorIdx];
                break;
            }

            default:
                ctx.pos++;
                break;
            }
        }

        _posX = px;
        _posY = py;

        /* Commit shadow buffer to canvas (skipped in batch mode) */
        _flush();
    }

    /* ------------------------------------------------------------------
       Public API
       ------------------------------------------------------------------ */
    return {
        init,
        clear,
        draw,
        beginBatch,
        endBatch,
        flush: _flush,

        getX,     getY,     setPos,
        getColor, setColor,
        getScale, setScale,
        getAngle, setAngle,
        qbColor,

        /* Expose palette for external use */
        palette: PALETTE_CSS,
    };

}());

/* ------------------------------------------------------------------
   Export for CommonJS / ES module bundlers while still working as a
   plain browser <script> include (where 'module' is not defined).
   ------------------------------------------------------------------ */
if (typeof module !== 'undefined' && module.exports) {
    module.exports = Draw;
}
