(* TPDraw Released on April 10, 2026 By RetroNick2020

   This unit demonstrates only the what is minimal possible with Draw Command.

   Please visit https://retronick2020.itch.io/vecdraw-v10-for-qbasicgwbasicqb64pe

   Download VecDraw to draw fancy graphics/text and import SVG's to create
   stunning graphics. Everything will export to Draw statements, nothing else
   required. VecDraw is Free software.

   Visit https://retronick2020.itch.io for other great programming utilities.


  TPDraw - QBASIC/QB64PE compatible DRAW command for Turbo Pascal Graph Unit
  ===========================================================================
  Implements the classic BASIC DRAW statement for use with Turbo Pascal Graph.
  Converted from Free Pascal Unit for compatibility.
  Draw - QBASIC/QB64PE compatible DRAW command for Turbo Pascal Graph Unit
  ===========================================================================
  Implements the classic BASIC DRAW statement for use with Turbo Pascal Graph.
  Converted from Free Pascal Unit for Turbo Pascal 7 compatibility.

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
    uses Graph, TPDraw;

    Draw('BM100,100 C14 S8 R50 D50 L50 U50');  { draw a yellow square }
    Draw('BM160,100 TA45 R50');                  { draw rotated line }
*)

unit TPCDraw;

interface

uses
  Graph;

{ Main DRAW command - parses and executes a DRAW command string }
procedure Draw(const Cmd: string);

{ Get/Set persistent DRAW state }
function  GetDrawX: Double;
function  GetDrawY: Double;
procedure SetDrawPos(AX, AY: Double);
function  GetDrawColor: LongInt;
procedure SetDrawColor(AColor: LongInt);
function  GetDrawScale: Double;
procedure SetDrawScale(AScale: Double);
function  GetDrawAngle: Double;
procedure SetDrawAngle(ADegrees: Double);

implementation

{ ---------------------------------------------------------------------------
  Persistent state across DRAW calls (mimics QB64PE's write_page state)
  --------------------------------------------------------------------------- }
var
  DrawPosX: Double;       { current X position }
  DrawPosY: Double;       { current Y position }
  DrawColor_: LongInt;    { current drawing color }
  DrawScaleVal: Double;   { scale factor (S/4) - default S4 = 1.0 }
  DrawTA: Double;         { current angle in degrees }
  DrawPosInitialized: Boolean;

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

function GetDrawColor: LongInt;
begin
  GetDrawColor := DrawColor_;
end;

procedure SetDrawColor(AColor: LongInt);
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
  Internal: Bresenham line drawing using PTCGraph PutPixel
  --------------------------------------------------------------------------- }
procedure DrawLine(X1, Y1, X2, Y2: Integer; Color: LongInt);
var
  dx, dy, sx, sy, err, e2: Integer;
begin
  dx := Abs(X2 - X1);
  dy := Abs(Y2 - Y1);
  if X1 < X2 then sx := 1 else sx := -1;
  if Y1 < Y2 then sy := 1 else sy := -1;
  err := dx - dy;

  while True do
  begin
    PutPixel(X1, Y1, Color);
    if (X1 = X2) and (Y1 = Y2) then Break;
    e2 := 2 * err;
    if e2 > -dy then begin Dec(err, dy); Inc(X1, sx); end;
    if e2 < dx  then begin Inc(err, dx); Inc(Y1, sy); end;
  end;
end;

{ ---------------------------------------------------------------------------
  Internal: Simple flood fill for the P command
  Uses a scanline stack-based approach.
  Stack is heap-allocated to avoid 64K stack limit.
  --------------------------------------------------------------------------- }
type
  TFillPoint = record
    X, Y: Integer;
  end;
  PFillStack = ^TFillStack;
  TFillStack = array[0..6383] of TFillPoint;

procedure FloodFill(StartX, StartY: Integer; FillColor, BorderColor: LongInt);
const
  MaxStack = 6384;
var
  Stack: PFillStack;
  SP: Integer;
  CX, CY: Integer;
  MaxX, MaxY: Integer;
  AX, AY: Integer;
  PC: LongInt;
  PopOK: Boolean;

  procedure Push(PushX, PushY: Integer);
  begin
    if SP < MaxStack then
    begin
      Stack^[SP].X := PushX;
      Stack^[SP].Y := PushY;
      Inc(SP);
    end;
  end;

  function CanFill(CFX, CFY: Integer): Boolean;
  var
    PCol: LongInt;
  begin
    if (CFX < 0) or (CFY < 0) or (CFX > MaxX) or (CFY > MaxY) then
    begin
      CanFill := False;
      Exit;
    end;
    PCol := GetPixel(CFX, CFY);
    CanFill := (PCol <> BorderColor) and (PCol <> FillColor);
  end;

begin
  MaxX := GetMaxX;
  MaxY := GetMaxY;
  if (StartX < 0) or (StartY < 0) or (StartX > MaxX) or (StartY > MaxY) then
    Exit;
  { If starting pixel is already the border or fill color, do nothing }
  if not CanFill(StartX, StartY) then
    Exit;

  New(Stack);
  SP := 0;
  Push(StartX, StartY);

  PopOK := SP > 0;
  while PopOK do
  begin
    Dec(SP);
    CX := Stack^[SP].X;
    CY := Stack^[SP].Y;

    if CanFill(CX, CY) then
    begin
      { Scan left to find the left edge of this span }
      while (CX > 0) and CanFill(CX - 1, CY) do
        Dec(CX);

      { Fill the span rightward, pushing adjacent rows }
      while (CX <= MaxX) and CanFill(CX, CY) do
      begin
        PutPixel(CX, CY, FillColor);
        if (CY > 0) and CanFill(CX, CY - 1) then
          Push(CX, CY - 1);
        if (CY < MaxY) and CanFill(CX, CY + 1) then
          Push(CX, CY + 1);
        Inc(CX);
      end;
    end;

    PopOK := SP > 0;
  end;

  Dispose(Stack);
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
procedure Draw(const Cmd: string);
var
  CmdStr: string;
  Pos_: Integer;
  Len_: Integer;

  { Direction vectors - same as QB64PE }
  VX, VY: Double;   { Up vector }
  HX, HY: Double;   { Right vector }
  EX, EY: Double;   { Up-Right vector (diagonal) }
  FX, FY: Double;   { Down-Right vector (diagonal) }

  PX, PY: Double;   { Current pixel position }
  PX2, PY2: Double; { Target position }
  XX, YY: Double;   { Movement delta }
  D: Double;        { Temp value }
  SinTA, CosTA: Double;
  Col: LongInt;
  PrefixB: Boolean; { B prefix active }
  PrefixN: Boolean; { N prefix active }

  { Aspect ratio - for PTCGraph we use 1:1 }
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
      { No digits found - check if we consumed a sign }
      if Negative then
      begin
        { A lone '-' with no digits: back up, treat as undefined }
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
    { Reset vectors to default orientation }
    VX := 0;   VY := -1;   { Up }
    EX := IR;  EY := -1;   { Up-Right }
    HX := IR;  HY :=  0;   { Right }
    FX := IR;  FY :=  1;   { Down-Right }

    if DrawTA <> 0 then
    begin
      Rad := DrawTA * (Pi / 180.0);
      SinTA := Sin(Rad);
      CosTA := Cos(Rad);

      { Rotate each vector: x' = x*cos + y*sin, y' = y*cos - x*sin }
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
    if not NumValid then Exit; { parse error }
    if NumUndefined then D := 1; { default distance = 1 }

    XX := XX * D;
    YY := YY * D;

    { Apply aspect ratio correction to X component }
    XX := XX * IR;

    { Apply scaling and compute target }
    PX2 := PX + XX * DrawScaleVal;
    PY2 := PY + YY * DrawScaleVal;

    { Draw line unless B prefix is set }
    if not PrefixB then
      DrawLine(QBRound(PX), QBRound(PY), QBRound(PX2), QBRound(PY2), Col);

    { Update position unless N prefix is set }
    if not PrefixN then
    begin
      PX := PX2;
      PY := PY2;
    end;

    { Clear prefixes after each drawing command }
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
    DrawPosX := GetMaxX div 2;
    DrawPosY := GetMaxY div 2;
    DrawPosInitialized := True;
  end;

  PX := DrawPosX;
  PY := DrawPosY;
  Col := DrawColor_;

  { PTCGraph = custom mode, always 1:1 aspect ratio }
  IR := 1.0;

  { Reset prefixes for this DRAW call }
  PrefixB := False;
  PrefixN := False;

  { Set up direction vectors and apply any existing rotation }
  ApplyRotation;

  { --- Main parse loop --- }
  Pos_ := 1;
  while Pos_ <= Len_ do
  begin
    { Skip whitespace and semicolons }
    if CmdStr[Pos_] in [' ', #9, ';'] then
    begin
      Inc(Pos_);
      { Continue equivalent - skip to top of while }
    end
    else
    begin

    C := PeekChar;

    case C of

      { === Direction commands === }
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
        { Skip whitespace after M }
        while (Pos_ <= Len_) and (CmdStr[Pos_] in [' ', #9]) do
          Inc(Pos_);

        if Pos_ > Len_ then Exit; { error: unexpected end }

        { Check for relative positioning (+/-) }
        IsRelative := CmdStr[Pos_] in ['+', '-'];

        { Parse X coordinate }
        PX2 := ParseNumber(NumValid, NumUndefined);
        if (not NumValid) or NumUndefined then Exit;

        { Expect comma }
        if (Pos_ > Len_) or (CmdStr[Pos_] <> ',') then Exit;
        Inc(Pos_);

        { Parse Y coordinate }
        PY2 := ParseNumber(NumValid, NumUndefined);
        if (not NumValid) or NumUndefined then Exit;

        if IsRelative then
        begin
          { Relative movement with rotation applied }
          XX := (PX2 * IR) * HX - (PY2 * IR) * VX;
          YY := PX2 * HY - PY2 * VY;
          PX2 := PX + XX * DrawScaleVal;
          PY2 := PY + YY * DrawScaleVal;
        end;
        { For absolute M, PX2/PY2 are already the target coords }

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

      { === B - Move without drawing prefix === }
      'B': begin
        PrefixB := True;
        Inc(Pos_);
      end;

      { === N - Draw without moving prefix === }
      'N': begin
        PrefixN := True;
        Inc(Pos_);
      end;

      { === C - Set color === }
      'C': begin
        Inc(Pos_);
        D := ParseNumber(NumValid, NumUndefined);
        if (not NumValid) or NumUndefined then Exit;
        Col := Trunc(D);
        DrawColor_ := Col;
      end;

      { === S - Set scale === }
      'S': begin
        Inc(Pos_);
        D := ParseNumber(NumValid, NumUndefined);
        if (not NumValid) or NumUndefined then Exit;
        if D < 0 then Exit; { invalid }
        DrawScaleVal := D / 4.0;
      end;

      { === A - Set angle (0-3 = 0/90/180/270 degrees) === }
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
          Exit; { invalid angle value }
        end;
        ApplyRotation;
      end;

      { === T - must be TA (Turn Angle) === }
      'T': begin
        Inc(Pos_);
        { Skip whitespace }
        while (Pos_ <= Len_) and (CmdStr[Pos_] in [' ', #9]) do
          Inc(Pos_);
        if Pos_ > Len_ then Exit;
        C := CmdStr[Pos_];
        if (C <> 'A') and (C <> 'a') then Exit; { must be TA }
        Inc(Pos_);
        D := ParseNumber(NumValid, NumUndefined);
        if (not NumValid) or NumUndefined then Exit;
        DrawTA := D;
        ApplyRotation;
      end;

      { === P - Paint (flood fill) === }
      'P': begin
        Inc(Pos_);
        D := ParseNumber(NumValid, NumUndefined);
        if (not NumValid) or NumUndefined then Exit;
        FillCol := Trunc(D);

        { Expect comma }
        if (Pos_ > Len_) or (CmdStr[Pos_] <> ',') then Exit;
        Inc(Pos_);

        D := ParseNumber(NumValid, NumUndefined);
        if (not NumValid) or NumUndefined then Exit;
        BorderCol := Trunc(D);

        { Flood fill at current position }
        FloodFill(QBRound(PX), QBRound(PY), LongInt(FillCol), LongInt(BorderCol));
        Col := LongInt(FillCol);
      end;

    else
      { Unknown command - skip it }
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

