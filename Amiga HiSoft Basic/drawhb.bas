' =================================================================
' drawhb.bas
' QBASIC/QB64PE compatible DRAW command
' HiSoft BASIC Professional port for the Amiga
' =================================================================
' Released April 2026 by RetroNick2020
'
' Please visit https://retronick2020.itch.io/vecdraw-v10-for-qbasicgwbasicqb64pe
' Download VecDraw to draw fancy graphics/text and import SVG files to create
' stunning graphics. Everything exports to DRAW statements. VecDraw is FREE.
' Visit https://retronick2020.itch.io for other great programming utilities.
'
' HOW TO USE
' ----------
' 1. In the HiSoft BASIC editor use File > Merge to load this file
'    into your program BEFORE any call to the DRAW routines, OR copy
'    the entire contents into your source manually.
' 2. Ensure the DECLARE lines below appear at the TOP of your merged
'    source (before any call to DrawGetX! / DrawGetY!).
' 3. See drawtest.bas for a complete working example.
'
' PUBLIC INTERFACE
' ----------------
'   CALL DrawInit(x!, y!)    - set start position, reset all state
'   CALL DrawSetPos(x!, y!)  - move pen (no draw)
'   CALL DrawSetColor(c%)    - set QB colour index 0-15
'   CALL DrawExec(cmd$)      - execute a DRAW string
'   DrawGetX!()              - return current pen X (FUNCTION)
'   DrawGetY!()              - return current pen Y (FUNCTION)
'
' AMIGA-SPECIFIC NOTES
' --------------------
' * Open a custom SCREEN with 4 bit-planes (16 colours) before drawing.
' * The P (PAINT / flood fill) command requires a WINDOW opened with
'   type 16 or higher (smart refresh).  See drawtest.bas.
' * Colour indices 0-15 map to the current Amiga screen palette.
'   Use PALETTE statements to match QB's CGA 16-colour set.
'
' PORTING NOTES (OpenWatcom C -> HiSoft BASIC Professional)
' ----------------------------------------------------------
'   _setpixel()          -> not needed; LINE handles segments
'   _setcolor()          -> colour passed directly to LINE / PAINT
'   draw_line()          -> LINE (x1,y1)-(x2,y2), colour
'   _floodfill()         -> PAINT (x,y), fill_colour, border_colour
'   _getvideoconfig()    -> SCREEN / WINDOW setup in calling program
'   parse context struct -> STATIC local vars + shared working vars
'   goto done            -> ok% flag terminates WHILE loop early
'   APPLY_ROTATION macro -> DrvApplyRot SUB
'   DO_MOVEMENT macro    -> DrvDoMove SUB
' =================================================================

' ---- Forward declarations (place these at the top of your program) ----
DECLARE FUNCTION DrawGetX!()
DECLARE FUNCTION DrawGetY!()

' ================================================================
' Shared state - accessible from all sub-programs via DIM SHARED.
' Declare at main-program level so every SUB can read/write them
' without needing individual SHARED statements.
' ================================================================

DIM SHARED drX!      ' current pen X position
DIM SHARED drY!      ' current pen Y position
DIM SHARED drClr%    ' QB colour index 0-15 (default 15 = white)
DIM SHARED drScl!    ' scale factor  (S value / 4;  S4 -> 1.0)
DIM SHARED drAng!    ' turn angle in degrees

' Working variables used during a DrawExec() call.
' Stored as shared to avoid passing many parameters between helpers.
DIM SHARED wpX!, wpY!          ' working pen position
DIM SHARED wpX2!, wpY2!        ' destination position
DIM SHARED wpClr%              ' working colour index
DIM SHARED wpPB%, wpPN%        ' B prefix / N prefix flags

' Direction unit vectors - recalculated after A or TA commands.
DIM SHARED dvVx!, dvVy!        ' "Up"
DIM SHARED dvHx!, dvHy!        ' "Right"
DIM SHARED dvEx!, dvEy!        ' "Up-Right" diagonal
DIM SHARED dvFx!, dvFy!        ' "Down-Right" diagonal

' ================================================================
SUB DrawInit(sx!, sy!)
' Initialise all DRAW state and set the starting pen position.
' Call once after opening your window, before the first DrawExec.
' ================================================================
    drX! = sx!  : drY! = sy!
    drClr% = 15              ' default white
    drScl! = 1.0             ' S4 = 1:1 scale
    drAng! = 0.0
END SUB

' ================================================================
SUB DrawSetPos(x!, y!)
' Move the pen to (x!, y!) without drawing.
' ================================================================
    drX! = x!  : drY! = y!
END SUB

' ================================================================
SUB DrawSetColor(c%)
' Set the active QB colour index (0-15).
' ================================================================
    drClr% = c% AND 15
END SUB

' ================================================================
FUNCTION DrawGetX!()
' Return the current pen X position.
' ================================================================
    DrawGetX! = drX!
END FUNCTION

' ================================================================
FUNCTION DrawGetY!()
' Return the current pen Y position.
' ================================================================
    DrawGetY! = drY!
END FUNCTION

' ================================================================
SUB DrvApplyRot()
' Internal: recalculate direction vectors from drAng!.
' Called automatically after A or TA commands in DrawExec.
'
' The direction vectors define what "Up", "Right" etc mean after
' rotation.  Without rotation: Up = (0,-1), Right = (1,0).
' ================================================================
    STATIC rd!, st!, ct!, tx!, ty!

    dvVx! = 0.0  : dvVy! = -1.0       ' Up     (V)
    dvEx! = 1.0  : dvEy! = -1.0       ' Up-Right diagonal (E)
    dvHx! = 1.0  : dvHy! =  0.0       ' Right  (H)
    dvFx! = 1.0  : dvFy! =  1.0       ' Down-Right diagonal (F)

    IF drAng! = 0.0 THEN EXIT SUB

    rd! = drAng! * (3.14159265358979 / 180.0)
    st! = SIN(rd!)
    ct! = COS(rd!)

    ' Rotate each direction vector by drAng! degrees.
    ' Rotation matrix: [ct  st]   (positive angle = clockwise on screen
    '                  [-st ct]    because Y increases downward)
    tx! = dvVx! : ty! = dvVy!
    dvVx! = tx! * ct! + ty! * st!  : dvVy! = ty! * ct! - tx! * st!

    tx! = dvHx! : ty! = dvHy!
    dvHx! = tx! * ct! + ty! * st!  : dvHy! = ty! * ct! - tx! * st!

    tx! = dvEx! : ty! = dvEy!
    dvEx! = tx! * ct! + ty! * st!  : dvEy! = ty! * ct! - tx! * st!

    tx! = dvFx! : ty! = dvFy!
    dvFx! = tx! * ct! + ty! * st!  : dvFy! = ty! * ct! - tx! * st!
END SUB

' ================================================================
SUB DrvSkipSp(s$, pos%, slen%)
' Internal: advance pos% past any space characters in s$.
' ================================================================
    WHILE pos% <= slen% AND MID$(s$, pos%, 1) = " "
        pos% = pos% + 1
    WEND
END SUB

' ================================================================
SUB DrvParseN(s$, pos%, slen%, numOut!, undef%)
' Internal: parse a decimal number (with optional leading sign)
' from s$ starting at pos%.
'
' On return:
'   pos%    - advanced past the number and any trailing spaces
'   numOut! - the parsed value (0.0 if none found)
'   undef%  - 1 if no digits were present (number absent)
'
' In DRAW strings a missing number defaults to 1 for movement
' commands and is a parse error for C / S / A / TA / P.
' ================================================================
    STATIC neg%, hasD%, ip!, fp!, fd!, done%, c$

    undef% = 0
    neg%   = 0  : hasD% = 0
    ip!    = 0.0 : fp!   = 0.0 : fd! = 1.0

    CALL DrvSkipSp(s$, pos%, slen%)
    IF pos% > slen% THEN
        undef% = 1 : numOut! = 0.0 : EXIT SUB
    END IF

    c$ = MID$(s$, pos%, 1)
    IF c$ = "-" THEN
        neg% = 1 : pos% = pos% + 1
    ELSEIF c$ = "+" THEN
        pos% = pos% + 1
    END IF

    ' --- integer part ---
    done% = 0
    WHILE pos% <= slen% AND done% = 0
        c$ = MID$(s$, pos%, 1)
        IF c$ >= "0" AND c$ <= "9" THEN
            ip! = ip! * 10.0 + (ASC(c$) - 48)
            hasD% = 1 : pos% = pos% + 1
        ELSE
            done% = 1
        END IF
    WEND

    ' --- optional fractional part ---
    IF pos% <= slen% THEN
        IF MID$(s$, pos%, 1) = "." THEN
            pos% = pos% + 1
            done% = 0
            WHILE pos% <= slen% AND done% = 0
                c$ = MID$(s$, pos%, 1)
                IF c$ >= "0" AND c$ <= "9" THEN
                    fp!   = fp! * 10.0 + (ASC(c$) - 48)
                    fd!   = fd! * 10.0
                    hasD% = 1 : pos% = pos% + 1
                ELSE
                    done% = 1
                END IF
            WEND
        END IF
    END IF

    IF hasD% = 0 THEN
        ' No digits found - back up past a lone '-' if present
        IF neg% = 1 THEN pos% = pos% - 1
        undef% = 1 : numOut! = 0.0 : EXIT SUB
    END IF

    CALL DrvSkipSp(s$, pos%, slen%)

    numOut! = ip! + fp! / fd!
    IF neg% THEN numOut! = -numOut!
END SUB

' ================================================================
SUB DrvDoMove(pos%, s$, slen%, ddx!, ddy!)
' Internal: execute one directional movement command (U/D/L/R/E/F/G/H).
'
' pos%         - index AFTER the command letter; advanced on return.
' s$, slen%    - full uppercase command string and its length.
' ddx!, ddy!   - pre-computed direction unit vector (e.g. dvVx!,dvVy!).
'
' Reads the optional distance, draws the line (unless B prefix set),
' updates wpX!/wpY! (unless N prefix set), then clears both prefixes.
' ================================================================
    STATIC nv!, ud%, tx2!, ty2!

    CALL DrvParseN(s$, pos%, slen%, nv!, ud%)
    IF ud% THEN nv! = 1.0          ' missing distance defaults to 1

    tx2! = wpX! + ddx! * nv! * drScl!
    ty2! = wpY! + ddy! * nv! * drScl!

    IF wpPB% = 0 THEN
        ' HiSoft BASIC: LINE (x1,y1)-(x2,y2), colour
        LINE (INT(wpX! + 0.5), INT(wpY! + 0.5))-(INT(tx2! + 0.5), INT(ty2! + 0.5)), wpClr%
    END IF

    IF wpPN% = 0 THEN
        wpX! = tx2! : wpY! = ty2!
    END IF

    wpPB% = 0 : wpPN% = 0
END SUB

' ================================================================
SUB DrawExec(cmd$)
' Execute a QBASIC/QB64PE-style DRAW command string.
' Must be called while a window is open and active.
'
' DRAW COMMAND REFERENCE
' ----------------------
'   U/D/L/R [n]     Move/draw Up/Down/Left/Right n pixels (default 1)
'   E/F/G/H [n]     Move/draw diagonally (UpR/DnR/DnL/UpL)
'   M [+|-]x,y      Move/draw to absolute or relative coordinates
'   B               Prefix: move without drawing
'   N               Prefix: draw without advancing pen
'   C n             Set colour (0-15)
'   S n             Set scale (S4 = 1:1 default)
'   A n             Angle preset: 0=0 deg, 1=90, 2=180, 3=270
'   TA n            Turn angle in degrees (-360 to 360)
'   P fill,border   Flood fill (smart-refresh window required)
' ================================================================
    STATIC s$, slen%, pos%, ok%, c$
    STATIC d!, ud%, isRel%
    STATIC mx!, my!, fc%, bc%

    s$ = UCASE$(cmd$)
    slen% = LEN(s$)
    IF slen% = 0 THEN EXIT SUB

    pos%   = 1
    wpX!   = drX!  : wpY!   = drY!
    wpClr% = drClr%
    wpPB%  = 0     : wpPN%  = 0
    ok%    = 1

    CALL DrvApplyRot

    ' ---- main parse loop ----
    WHILE pos% <= slen% AND ok%

        c$ = MID$(s$, pos%, 1)

        ' skip whitespace and semicolons
        IF c$ = " " OR c$ = ";" THEN
            pos% = pos% + 1
        ELSE

        SELECT CASE c$

        ' ---- 8 directional movement commands ----
        CASE "U"
            pos% = pos% + 1
            CALL DrvDoMove(pos%, s$, slen%, dvVx!, dvVy!)

        CASE "D"
            pos% = pos% + 1
            CALL DrvDoMove(pos%, s$, slen%, -dvVx!, -dvVy!)

        CASE "L"
            pos% = pos% + 1
            CALL DrvDoMove(pos%, s$, slen%, -dvHx!, -dvHy!)

        CASE "R"
            pos% = pos% + 1
            CALL DrvDoMove(pos%, s$, slen%, dvHx!, dvHy!)

        CASE "E"
            pos% = pos% + 1
            CALL DrvDoMove(pos%, s$, slen%, dvEx!, dvEy!)

        CASE "F"
            pos% = pos% + 1
            CALL DrvDoMove(pos%, s$, slen%, dvFx!, dvFy!)

        CASE "G"
            pos% = pos% + 1
            CALL DrvDoMove(pos%, s$, slen%, -dvEx!, -dvEy!)

        CASE "H"
            pos% = pos% + 1
            CALL DrvDoMove(pos%, s$, slen%, -dvFx!, -dvFy!)

        ' ---- B prefix: next move does not draw ----
        CASE "B"
            wpPB% = 1 : pos% = pos% + 1

        ' ---- N prefix: pen stays after drawing ----
        CASE "N"
            wpPN% = 1 : pos% = pos% + 1

        ' ---- M: move/draw to absolute or relative position ----
        CASE "M"
            pos% = pos% + 1
            CALL DrvSkipSp(s$, pos%, slen%)
            IF pos% > slen% THEN
                ok% = 0
            ELSE
                isRel% = 0
                c$ = MID$(s$, pos%, 1)
                IF c$ = "+" OR c$ = "-" THEN isRel% = 1

                CALL DrvParseN(s$, pos%, slen%, wpX2!, ud%)
                IF ud% THEN ok% = 0
            END IF

            IF ok% THEN
                IF pos% > slen% THEN
                    ok% = 0
                ELSEIF MID$(s$, pos%, 1) <> "," THEN
                    ok% = 0
                ELSE
                    pos% = pos% + 1
                    CALL DrvParseN(s$, pos%, slen%, wpY2!, ud%)
                    IF ud% THEN ok% = 0
                END IF
            END IF

            IF ok% THEN
                IF isRel% THEN
                    ' Apply rotation to relative offset
                    mx! = wpX2! * dvHx! - wpY2! * dvVx!
                    my! = wpX2! * dvHy! - wpY2! * dvVy!
                    wpX2! = wpX! + mx! * drScl!
                    wpY2! = wpY! + my! * drScl!
                END IF
                IF wpPB% = 0 THEN
                    LINE (INT(wpX!+0.5),INT(wpY!+0.5))-(INT(wpX2!+0.5),INT(wpY2!+0.5)),wpClr%
                END IF
                IF wpPN% = 0 THEN wpX! = wpX2! : wpY! = wpY2!
                wpPB% = 0 : wpPN% = 0
            END IF

        ' ---- C: set draw colour ----
        CASE "C"
            pos% = pos% + 1
            CALL DrvParseN(s$, pos%, slen%, d!, ud%)
            IF ud% THEN
                ok% = 0
            ELSE
                wpClr% = INT(d!) AND 15
                drClr% = wpClr%
            END IF

        ' ---- S: set scale factor ----
        CASE "S"
            pos% = pos% + 1
            CALL DrvParseN(s$, pos%, slen%, d!, ud%)
            IF ud% OR d! < 0.0 THEN
                ok% = 0
            ELSE
                drScl! = d! / 4.0
            END IF

        ' ---- A: angle preset (0-3) ----
        CASE "A"
            pos% = pos% + 1
            CALL DrvParseN(s$, pos%, slen%, d!, ud%)
            IF ud% THEN
                ok% = 0
            ELSE
                SELECT CASE INT(d!)
                CASE 0 : drAng! = 0.0
                CASE 1 : drAng! = 90.0
                CASE 2 : drAng! = 180.0
                CASE 3 : drAng! = 270.0
                CASE REMAINDER : ok% = 0
                END SELECT
                IF ok% THEN CALL DrvApplyRot
            END IF

        ' ---- T: must be TA (turn angle in degrees) ----
        CASE "T"
            pos% = pos% + 1
            CALL DrvSkipSp(s$, pos%, slen%)
            IF pos% > slen% THEN
                ok% = 0
            ELSEIF MID$(s$, pos%, 1) <> "A" THEN
                ok% = 0
            ELSE
                pos% = pos% + 1
                CALL DrvParseN(s$, pos%, slen%, d!, ud%)
                IF ud% THEN
                    ok% = 0
                ELSE
                    drAng! = d!
                    CALL DrvApplyRot
                END IF
            END IF

        ' ---- P: flood fill ----
        ' IMPORTANT: requires a smart-refresh window (type 16+).
        ' HiSoft BASIC: PAINT (x,y), fill_colour, border_colour
        CASE "P"
            pos% = pos% + 1
            CALL DrvParseN(s$, pos%, slen%, d!, ud%)
            IF ud% THEN
                ok% = 0
            ELSE
                fc% = INT(d!) AND 15
                IF pos% > slen% THEN
                    ok% = 0
                ELSEIF MID$(s$, pos%, 1) <> "," THEN
                    ok% = 0
                ELSE
                    pos% = pos% + 1
                    CALL DrvParseN(s$, pos%, slen%, d!, ud%)
                    IF ud% THEN
                        ok% = 0
                    ELSE
                        bc% = INT(d!) AND 15
                        ' HiSoft BASIC PAINT syntax:
                        ' PAINT [STEP](x,y) [,paint_colour][,border_colour]
                        PAINT (INT(wpX!+0.5), INT(wpY!+0.5)), fc%, bc%
                        wpClr% = fc% : drClr% = fc%
                    END IF
                END IF
            END IF

        ' ---- Unknown character: skip ----
        CASE REMAINDER
            pos% = pos% + 1

        END SELECT

        END IF  ' end non-whitespace branch

    WEND  ' end main parse loop

    ' Save updated pen position back to persistent state
    drX! = wpX! : drY! = wpY!
END SUB
