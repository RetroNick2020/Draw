(* TestTD Released on April 10, 2026 By RetroNick2020

   This program demonstrates only the what is minimal possible with Draw Command.

   Please visit https://retronick2020.itch.io/vecdraw-v10-for-qbasicgwbasicqb64pe

   Download VecDraw to draw fancy graphics/text and import SVG's to create
   stunning graphics. Everything will export to Draw statements, nothing else
   required. VecDraw is Free softwware.

   Visit https://retronick2020.itch.io for other great programming utilities.
*)

program TestTD;

uses
  Graph, Crt, PTCDraw;

var
  gd, gm: Integer;
begin
  { Initialize Graph - 640x480x16 }
  gd := VGA;
  gm := VGAHi;
  InitGraph(gd, gm, 'c:\tp7\bgi');

  { --- Example 1: Square outline --- }
  SetDrawPos(20, 20);
  SetDrawColor(14);  { Yellow }
  Draw('R40 D40 L40 U40');

  { --- Example 2: Filled diamond using diagonals --- }
  SetDrawPos(120, 50);
  SetDrawColor(10);  { Light Green }
  Draw('E20 F20 G20 H20');
  Draw('P10,10');  { Fill with green, border green }

  { --- Example 3: Rotated square using TA45 --- }
  SetDrawPos(230, 50);
  SetDrawColor(9);   { Light Blue }
  Draw('TA45 R20 D20 L20 U20 TA0');

  { --- Example 4: Closed triangle - R40 G20 H20 --- }
  { R moves 40 right. G moves 20 diagonal down-left. }
  { H moves 20 diagonal up-left. Total X: 40-20-20=0 }
  SetDrawPos(20, 100);
  SetDrawColor(12);  { Light Red }
  Draw('R40 G20 H20');

  { --- Example 5: Scaled triangle (S8 = 2x scale) --- }
  SetDrawPos(100, 100);
  SetDrawColor(13);  { Light Magenta }
  Draw('S8 R20 G10 H10 S4');

  { --- Example 6: Star using N prefix (draw without moving) --- }
  SetDrawPos(230, 120);
  SetDrawColor(11);  { Light Cyan }
  Draw('NU15 ND15 NL15 NR15 NE10 NF10 NG10 NH10');

  { --- Example 7: Absolute M and relative M --- }
  SetDrawColor(15);  { White }
  Draw('BM20,170 R30 M+0,+20 L30 M20,170');  { Rectangle via M }

  { --- Example 8: 90-degree rotations with A command --- }
  SetDrawPos(120, 170);
  SetDrawColor(14);
  Draw('A0 R15');       { Right }
  SetDrawPos(120, 170);
  Draw('A1 R15');       { Up (90 deg) }
  SetDrawPos(120, 170);
  Draw('A2 R15');       { Left (180 deg) }
  SetDrawPos(120, 170);
  Draw('A3 R15');       { Down (270 deg) }
  Draw('A0');           { Reset rotation }

  { Wait for keypress }
  ReadKey;
  CloseGraph;
end.
