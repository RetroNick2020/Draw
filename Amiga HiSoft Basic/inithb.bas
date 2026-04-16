' =================================================================
' inithb.bas
' DRAW command demo for HiSoft BASIC Professional (Amiga)
' =================================================================
' Released April 2026 by RetroNick2020
'
' Please visit https://retronick2020.itch.io/vecdraw-v10-for-qbasicgwbasicqb64pe
' Download VecDraw to draw fancy graphics/text and import SVG files to create
' stunning graphics. Everything exports to DRAW statements. VecDraw is FREE.
' Visit https://retronick2020.itch.io for other great programming utilities.
'
' HOW TO BUILD
' ------------
' Combine this file with drawhb.bas or run the existing testhb.bas
' The drawhb.bas file is meant to be attached to any programs you are working
' to give you the DrawExec command
'
' SCREEN LAYOUT (640 x 200, 16 colours)
' ----------------------------------------
' Yellow square      top-left area
' Red filled diamond centre-left area
' Cyan rotated square  right side
' Green star         lower area
'
' Press any key to exit.
' =================================================================

' ---- Forward declarations for functions defined in owdraw.bas ----
DECLARE FUNCTION DrawGetX!()
DECLARE FUNCTION DrawGetY!()

' =================================================================
' Screen and window setup
' =================================================================

' Open a custom hi-res screen: 640 x 200, 4 bit-planes = 16 colours.
'   SCREEN id, width, height, bit-planes, mode
'   mode 2 = high-resolution, non-interlaced
SCREEN 1, 640, 200, 4, 2

' Open a full-screen smart-refresh window on screen 1.
'   WINDOW OPEN id, title, (x1,y1)-(x2,y2), type, screen-id
'   type 16 = smart refresh  <-- REQUIRED for the P (PAINT) command
'   No gadgets added (no size/drag/depth/close) for clean display.
'
'   NOTE: If PAINT does not work, try type 16+2+4 (= 22, adds
'   drag-bar and depth gadget) or type 32 (super-bitmap).
WINDOW OPEN 1, "DRAW Demo", (0,0)-(639,197), 16, 1

' Make window 1 the active output window for all graphics commands.
WINDOW OUTPUT 1

' =================================================================
' Set up a QB CGA-compatible 16-colour palette.
' PALETTE index, red, green, blue  (values 0.0 to 1.0)
' Amiga OCS/ECS hardware rounds each gun to 4 bits (16 levels).
' =================================================================
PALETTE  0,  0.000, 0.000, 0.000   ' 0  Black
PALETTE  1,  0.000, 0.000, 0.667   ' 1  Dark Blue
PALETTE  2,  0.000, 0.667, 0.000   ' 2  Dark Green
PALETTE  3,  0.000, 0.667, 0.667   ' 3  Dark Cyan
PALETTE  4,  0.667, 0.000, 0.000   ' 4  Dark Red
PALETTE  5,  0.667, 0.000, 0.667   ' 5  Dark Magenta
PALETTE  6,  0.667, 0.333, 0.000   ' 6  Brown
PALETTE  7,  0.667, 0.667, 0.667   ' 7  Light Grey
PALETTE  8,  0.333, 0.333, 0.333   ' 8  Dark Grey
PALETTE  9,  0.333, 0.333, 1.000   ' 9  Blue
PALETTE 10,  0.333, 1.000, 0.333   ' 10 Green
PALETTE 11,  0.333, 1.000, 1.000   ' 11 Cyan
PALETTE 12,  1.000, 0.333, 0.333   ' 12 Red
PALETTE 13,  1.000, 0.333, 1.000   ' 13 Magenta
PALETTE 14,  1.000, 1.000, 0.333   ' 14 Yellow
PALETTE 15,  1.000, 1.000, 1.000   ' 15 White

' Clear window to black (foreground=white, background=black)
COLOR 15, 0
CLS

' =================================================================
' Demo drawings
'
' Coordinate reminders for a 640x200 screen:
'   X range: 0 - 639
'   Y range: 0 - 197  (top-left = 0,0)
'   Origin is top-left; Y increases downward (same as QB).
' =================================================================

' ---- Yellow square ----
' S8 = scale 2.0  =>  R50 draws 100 px,  square is 100 x 100.
' BM80,15 = blind move to (80,15) without drawing.
DrawInit 0.0, 0.0
DrawExec "BM80,15 C14 S8 R50 D50 L50 U50"

' ---- Red diamond ----
' Default S4 = scale 1.0.  Each diagonal side is 30 px.
' Starting at (320,60): E goes up-right, F down-right, G down-left,
' H up-left, returning to start -> a closed diamond.
DrawSetPos 320.0, 60.0
DrawSetColor 12
DrawExec "E30 F30 G30 H30"

' Flood fill the diamond interior with red (colour 12).
' Point (350,60) is the geometric centre of the diamond.
' PAINT requires the window to be smart-refresh (type 16 or higher).
DrawExec "BM350,60 P12,12"

' ---- Cyan rotated square (45 degrees) ----
' TA45 rotates the coordinate frame 45 degrees clockwise.
' S4 = scale 1.0;  each side is 40 px.
DrawExec "BM500,60 C11 TA45 S4 R40 D40 L40 U40"

' ---- Green star using N (no-advance) prefix ----
' N prefix draws each ray but leaves the pen at the centre,
' so all 8 rays radiate from the same point (160,155).
' S4 = scale 1.0;  each ray is 20 px long.
DrawExec "BM160,155 C10 S4"
DrawExec "NU20 ND20 NL20 NR20 NE20 NF20 NG20 NH20"

' =================================================================
' Wait for the user to press any key, then clean up.
' =================================================================
DO : LOOP UNTIL INKEY$ <> ""

WINDOW CLOSE 1
SCREEN CLOSE 1
