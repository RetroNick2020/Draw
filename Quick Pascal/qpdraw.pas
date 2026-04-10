(* QPDraw Released on April 10, 2026 By RetroNick2020

   This unit demonstrates only the what is minimal possible with Draw Command.

   Please visit https://retronick2020.itch.io/vecdraw-v10-for-qbasicgwbasicqb64pe

   Download VecDraw to draw fancy graphics/text and import SVG's to create
   stunning graphics. Everything will export to Draw statements, nothing else
   required. VecDraw is Free software.

   Visit https://retronick2020.itch.io for other great programming utilities.


  QPDraw - QBASIC/QB64PE compatible DRAW command for Quick Pascal Graph Unit
  ===========================================================================
  Implements the classic BASIC DRAW statement for use with Quick Pascal MSGraph.
  Converted from Turbo Pascal Unit for compatibility.

  Supported commands:
    Movement/Drawing:
      U[n]  - Up n units (default 1)
      D[n]  - Down n units
      L[n]  - Left n units
      R[n]  - Right n units
      E[n]  - Up-Right (diagonal)
      F[n]  - Down-Right (diagonal)
      G[n]  - Down-Left (diagonal)
      H[n]  - Up-Left (diagonal)
      M[+/-]x,y - Move to absolute x,y or relative +x,+y / -x,-y

    Prefixes (apply to the next movement command only):
      B     - Move without drawing
      N     - Draw and return to original position

    Color/Style:
      Cn    - Set drawing color to n
      Sn    - Set scale factor (default 4 = 1 pixel per unit)
      An    - Set angle: 0=0, 1=90, 2=180, 3=270 degrees
      TAn   - Set angle to n degrees (-360 to 360)
      Pf,b  - Paint (flood fill) with fill color f, border color b

  Whitespace and semicolons are ignored between commands.

  Usage:
    uses MSGraph, QPDraw;

    Draw('BM100,100 C14 S8 R50 D50 L50 U50');  { draw a yellow square }
    Draw('BM160,100 TA45 R50');                { draw rotated line }

*)


unit QPDraw;
{$N+}  { Enable 8087 numeric coprocessor for Double type }

interface

uses
  MSGraph;

{ Main DRAW command - parses and executes a DRAW command string }
procedure Draw(Cmd: string);

{ Get/Set persistent DRAW state }
function  GetDrawX: Double;
function  GetDrawY: Double;
procedure SetDrawPos(AX, AY: Double);
function  GetDrawColor: Integer;
procedure SetDrawColor(AColor: Integer);
function  GetDrawScale: Double;
procedure SetDrawScale(AScale: Double);
function  GetDrawAngle: Double;
procedure SetDrawAngle(ADegrees: Double);

implementation

{ ---------------------------------------------------------------------------
  Persistent state across DRAW calls
  --------------------------------------------------------------------------- }
var
  DrawPosX: Double;
  DrawPosY: Double;
  DrawColor_: Integer;
  DrawScaleVal: Double;
  DrawTA: Double;
  DrawPosInitialized: Boolean;

{ ---------------------------------------------------------------------------
  Helper: Get screen dimensions from MSGraph _GetVideoConfig
  --------------------------------------------------------------------------- }
function ScreenMaxX: Integer;
var
  vc: _VideoConfig;
begin
  _GetVideoConfig(vc);
  ScreenMaxX := vc.NumXPixels - 1;
end;

function ScreenMaxY: Integer;
var
  vc: _VideoConfig;
begin
  _GetVideoConfig(vc);
  ScreenMaxY := vc.NumYPixels - 1;
end;

{ ---------------------------------------------------------------------------
  Accessors
  --------------------------------------------------------------------------- }
function GetDrawX: Double;
begin
  GetDrawX := DrawPosX;
end;

function GetDrawY: Double;
begin
  GetDrawY := DrawPosY;
end;

procedure SetDrawPos(AX, AY: Double);
begin
  DrawPosX := AX;
  DrawPosY := AY;
  DrawPosInitialized := True;
end;

function GetDrawColor: Integer;
begin
  GetDrawColor := DrawColor_;
end;

procedure SetDrawColor(AColor: Integer);
begin
  DrawColor_ := AColor;
end;

function GetDrawScale: Double;
begin
  GetDrawScale := DrawScaleVal;
end;

procedure SetDrawScale(AScale: Double);
begin
  DrawScaleVal := AScale;
end;

function GetDrawAngle: Double;
begin
  GetDrawAngle := DrawTA;
end;

procedure SetDrawAngle(ADegrees: Double);
begin
  DrawTA := ADegrees;
end;

{ ---------------------------------------------------------------------------
  Internal: Bresenham line drawing using MSGraph _SetColor/_SetPixel
  We use our own Bresenham rather than _MoveTo/_LineTo because
  LineTo typically omits the last pixel.
  --------------------------------------------------------------------------- }
procedure DrawLine(X1, Y1, X2, Y2: Integer; Color: Integer);
var
  dx, dy, sx, sy, err, e2: Integer;
  wTrue : boolean;
begin
  _SetColor(Color);
  dx := Abs(X2 - X1);
  dy := Abs(Y2 - Y1);
  if X1 < X2 then sx := 1 else sx := -1;
  if Y1 < Y2 then sy := 1 else sy := -1;
  err := dx - dy;

  wTrue:=True;
  while wTrue do
  begin
    _SetPixel(X1, Y1);
    if (X1 = X2) and (Y1 = Y2) then
    begin
      wTrue:=False
    end
    else
    begin
      e2 := 2 * err;
      if e2 > -dy then begin Dec(err, dy); Inc(X1, sx); end;
      if e2 < dx  then begin Inc(err, dx); Inc(Y1, sy); end;
    end;
  end;
end;

{ ---------------------------------------------------------------------------
  QB64PE-compatible rounding
  --------------------------------------------------------------------------- }
function QBRound(D: Double): Integer;
begin
  if D >= 0 then
    QBRound := Trunc(D + 0.5)
  else
    QBRound := Trunc(D - 0.5);
end;

{ ---------------------------------------------------------------------------
  Main DRAW parser
  --------------------------------------------------------------------------- }
procedure Draw(Cmd: string);
var
  CmdStr: string;
  Pos_: Integer;
  Len_: Integer;

  { Direction vectors }
  VX, VY: Double;
  HX, HY: Double;
  EX, EY: Double;
  FX, FY: Double;

  PX, PY: Double;
  PX2, PY2: Double;
  XX, YY: Double;
  D: Double;
  SinTA, CosTA: Double;
  Col: Integer;
  PrefixB: Boolean;
  PrefixN: Boolean;

  IR: Double;

  { --- Helper: peek current char (uppercase), #0 if past end --- }
  function PeekChar: Char;
  var
    Ch: Char;
  begin
    if Pos_ > Len_ then
      PeekChar := #0
    else
    begin
      Ch := CmdStr[Pos_];
      if (Ch >= 'a') and (Ch <= 'z') then
	Ch := Chr(Ord(Ch) - 32);
      PeekChar := Ch;
    end;
  end;

  { --- Helper: advance and return next char (uppercase) --- }
  function NextChar: Char;
  begin
    Inc(Pos_);
    NextChar := PeekChar;
  end;

  { --- Helper: skip whitespace and semicolons --- }
  procedure SkipWhitespace;
  begin
    while (Pos_ <= Len_) and (CmdStr[Pos_] in [' ', #9, ';']) do
      Inc(Pos_);
  end;

  { --- Helper: parse a number from the command string --- }
  function ParseNumber(var Valid: Boolean; var Undefined: Boolean): Double;
  var
    Negative: Boolean;
    HasDigit: Boolean;
    IntPart: Double;
    FracPart: Double;
    FracDiv: Double;
    Ch: Char;
  begin
    Valid := True;
    Undefined := False;
    ParseNumber := 0;
    Negative := False;
    HasDigit := False;

    { Skip whitespace before number }
    while (Pos_ <= Len_) and (CmdStr[Pos_] in [' ', #9]) do
      Inc(Pos_);

    if Pos_ > Len_ then
    begin
      Undefined := True;
      Exit;
    end;

    Ch := CmdStr[Pos_];

    { Check for sign }
    if Ch = '-' then
    begin
      Negative := True;
      Inc(Pos_);
    end
    else if Ch = '+' then
      Inc(Pos_);

    { Parse integer part }
    IntPart := 0;
    while (Pos_ <= Len_) and (CmdStr[Pos_] >= '0') and (CmdStr[Pos_] <= '9') do
    begin
      IntPart := IntPart * 10 + (Ord(CmdStr[Pos_]) - Ord('0'));
      HasDigit := True;
      Inc(Pos_);
    end;

    { Parse fractional part }
    FracPart := 0;
    FracDiv := 1;
    if (Pos_ <= Len_) and (CmdStr[Pos_] = '.') then
    begin
      Inc(Pos_);
      while (Pos_ <= Len_) and (CmdStr[Pos_] >= '0') and (CmdStr[Pos_] <= '9') do
      begin
	      FracPart := FracPart * 10 + (Ord(CmdStr[Pos_]) - Ord('0'));
	      FracDiv := FracDiv * 10;
	      HasDigit := True;
	      Inc(Pos_);
      end;
    end;

    if not HasDigit then
    begin
      if Negative then
      begin
	      Dec(Pos_);
	      Undefined := True;
      end
      else
	      Undefined := True;
      Exit;
    end;

    ParseNumber := IntPart + FracPart / FracDiv;
    if Negative then
      ParseNumber := -(IntPart + FracPart / FracDiv);

    { Skip trailing whitespace }
    while (Pos_ <= Len_) and (CmdStr[Pos_] in [' ', #9]) do
      Inc(Pos_);
  end;

  { --- Helper: rotate all direction vectors by DrawTA --- }
  procedure ApplyRotation;
  var
    Rad: Double;
  begin
    VX := 0;   VY := -1;
    EX := IR;  EY := -1;
    HX := IR;  HY :=  0;
    FX := IR;  FY :=  1;

    if DrawTA <> 0 then
    begin
      Rad := DrawTA * (Pi / 180.0);
      SinTA := Sin(Rad);
      CosTA := Cos(Rad);

      PX2 := VX; PY2 := VY;
      VX := PX2 * CosTA + PY2 * SinTA;
      VY := PY2 * CosTA - PX2 * SinTA;

      PX2 := HX; PY2 := HY;
      HX := PX2 * CosTA + PY2 * SinTA;
      HY := PY2 * CosTA - PX2 * SinTA;

      PX2 := EX; PY2 := EY;
      EX := PX2 * CosTA + PY2 * SinTA;
      EY := PY2 * CosTA - PX2 * SinTA;

      PX2 := FX; PY2 := FY;
      FX := PX2 * CosTA + PY2 * SinTA;
      FY := PY2 * CosTA - PX2 * SinTA;
    end;
  end;

  { --- Handle U/D/L/R/E/F/G/H movement --- }
  procedure DoMovement;
  var
    NumValid, NumUndefined: Boolean;
  begin
    Inc(Pos_);
    D := ParseNumber(NumValid, NumUndefined);
    if not NumValid then Exit;
    if NumUndefined then D := 1;

    XX := XX * D;
    YY := YY * D;
    XX := XX * IR;

    PX2 := PX + XX * DrawScaleVal;
    PY2 := PY + YY * DrawScaleVal;

    if not PrefixB then
      DrawLine(QBRound(PX), QBRound(PY), QBRound(PX2), QBRound(PY2), Col);

    if not PrefixN then
    begin
      PX := PX2;
      PY := PY2;
    end;

    PrefixB := False;
    PrefixN := False;
  end;

var
  C: Char;
  NumValid, NumUndefined: Boolean;
  IsRelative: Boolean;
  FillCol, BorderCol: Integer;
  AngleVal: Integer;
begin
  CmdStr := Cmd;
  Len_ := Length(CmdStr);
  if Len_ = 0 then Exit;

  { Initialize position from persistent state if not yet set }
  if not DrawPosInitialized then
  begin
    DrawPosX := ScreenMaxX div 2;
    DrawPosY := ScreenMaxY div 2;
    DrawPosInitialized := True;
  end;

  PX := DrawPosX;
  PY := DrawPosY;
  Col := DrawColor_;

  IR := 1.0;

  PrefixB := False;
  PrefixN := False;

  ApplyRotation;

  { --- Main parse loop --- }
  Pos_ := 1;
  while Pos_ <= Len_ do
  begin
    { Skip whitespace and semicolons }
    if CmdStr[Pos_] in [' ', #9, ';'] then
    begin
      Inc(Pos_);
    end
    else
    begin
      C := PeekChar;
      case C of
	'U': begin XX := VX;  YY := VY;  DoMovement; end;
	'D': begin XX := -VX; YY := -VY; DoMovement; end;
	'L': begin XX := -HX; YY := -HY; DoMovement; end;
	'R': begin XX := HX;  YY := HY;  DoMovement; end;
	'E': begin XX := EX;  YY := EY;  DoMovement; end;
	'F': begin XX := FX;  YY := FY;  DoMovement; end;
	'G': begin XX := -EX; YY := -EY; DoMovement; end;
	'H': begin XX := -FX; YY := -FY; DoMovement; end;

	{ === M - Move to absolute or relative position === }
	'M': begin
		    Inc(Pos_);
		    while (Pos_ <= Len_) and (CmdStr[Pos_] in [' ', #9]) do
		      Inc(Pos_);
		if Pos_ > Len_ then Exit;
		      IsRelative := CmdStr[Pos_] in ['+', '-'];
		PX2 := ParseNumber(NumValid, NumUndefined);
		      if (not NumValid) or NumUndefined then Exit;
		if (Pos_ > Len_) or (CmdStr[Pos_] <> ',') then Exit;
		Inc(Pos_);
		PY2 := ParseNumber(NumValid, NumUndefined);
		      if (not NumValid) or NumUndefined then Exit;
		if IsRelative then
		begin
		  XX := (PX2 * IR) * HX - (PY2 * IR) * VX;
			YY := PX2 * HY - PY2 * VY;
			PX2 := PX + XX * DrawScaleVal;
		  PY2 := PY + YY * DrawScaleVal;
		      end;
		      if not PrefixB then
			DrawLine(QBRound(PX), QBRound(PY), QBRound(PX2), QBRound(PY2), Col);
		      if not PrefixN then
		      begin
			PX := PX2;
			PY := PY2;
		end;
		      PrefixB := False;
		      PrefixN := False;
	      end;
       'B': begin
		  PrefixB := True;
		  Inc(Pos_);
	   end;
      'N': begin
		  PrefixN := True;
		  Inc(Pos_);
	  end;
      'C': begin
		Inc(Pos_);
		  D := ParseNumber(NumValid, NumUndefined);
		  if (not NumValid) or NumUndefined then Exit;
		  Col := Trunc(D);
		  DrawColor_ := Col;
	  end;
      'S': begin
		Inc(Pos_);
		  D := ParseNumber(NumValid, NumUndefined);
		if (not NumValid) or NumUndefined then Exit;
		  if D < 0 then Exit;
		  DrawScaleVal := D / 4.0;
	  end;
      'A': begin
	    Inc(Pos_);
		  D := ParseNumber(NumValid, NumUndefined);
		if (not NumValid) or NumUndefined then Exit;
		  AngleVal := Trunc(D);
		  case AngleVal of
		    0: DrawTA := 0;
		    1: DrawTA := 90;
		  2: DrawTA := 180;
		    3: DrawTA := 270;
		else
		    Exit;
		  end;
		  ApplyRotation;
	  end;
      'T': begin
		Inc(Pos_);
		  while (Pos_ <= Len_) and (CmdStr[Pos_] in [' ', #9]) do
		  Inc(Pos_);
		    if Pos_ > Len_ then Exit;
		C := CmdStr[Pos_];
		    if (C <> 'A') and (C <> 'a') then Exit;
		Inc(Pos_);
		    D := ParseNumber(NumValid, NumUndefined);
		if (not NumValid) or NumUndefined then Exit;
		    DrawTA := D;
		ApplyRotation;
	    end;
      { === P - Paint (flood fill) using MSGraph _FloodFill === }
      'P': begin
		  Inc(Pos_);
		D := ParseNumber(NumValid, NumUndefined);
		if (not NumValid) or NumUndefined then Exit;
		  FillCol := Trunc(D);
		if (Pos_ > Len_) or (CmdStr[Pos_] <> ',') then Exit;
		  Inc(Pos_);
	    D := ParseNumber(NumValid, NumUndefined);
		  if (not NumValid) or NumUndefined then Exit;
	    BorderCol := Trunc(D);
		{ Use MSGraph's built-in flood fill }
		  { _SetColor sets the fill color, _FloodFill fills to boundary }
		  _SetColor(FillCol);
		  _FloodFill(QBRound(PX), QBRound(PY), BorderCol);
		  Col := FillCol;
	  end;
    else
      Inc(Pos_);
    end;
    end; { else branch of whitespace check }
  end;

  { Save position back to persistent state }
  DrawPosX := PX;
  DrawPosY := PY;
  DrawPosInitialized := True;
end;

{ Initialization }
begin
  DrawPosX := 0;
  DrawPosY := 0;
  DrawColor_ := 15;
  DrawScaleVal := 1.0;
  DrawTA := 0;
  DrawPosInitialized := False;
end.
