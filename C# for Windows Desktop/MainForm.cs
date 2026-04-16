// MainForm.cs
// ============================================================================
// WinForms host for DrawEngine - retro phosphor-green CRT aesthetic
// ============================================================================

using System;
using System.Drawing;
using System.Drawing.Drawing2D;
using System.Windows.Forms;

namespace DrawDemo
{
    public class MainForm : Form
    {
        // ---- UI controls ----
        private Panel      _canvasPanel;
        private PictureBox _canvas;
        private TextBox    _cmdInput;
        private Button     _btnExecute;
        private Button     _btnDemo;
        private Button     _btnClear;
        private Label      _lblPos;
        private Label      _lblTitle;
        private Panel      _sidebar;
        private RichTextBox _log;
        private Label      _lblCmd;
        private Label      _lblLog;

        // ---- engine ----
        private Bitmap     _bmp;
        private DrawEngine _engine;

        private const int CANVAS_W = 640;
        private const int CANVAS_H = 480;

        // ---- palette for the CRT theme ----
        private static readonly Color BgDark    = Color.FromArgb(10,  12,  10 );
        private static readonly Color PhosGreen = Color.FromArgb(57,  255, 20 );  // neon green
        private static readonly Color PhosGreenDim = Color.FromArgb(30, 140, 15);
        private static readonly Color PanelBg   = Color.FromArgb(18,  22,  18 );
        private static readonly Color BorderCol = Color.FromArgb(40,  80,  40 );

        public MainForm()
        {
            InitBitmap();
            BuildUI();
            RunDemo();
        }

        // -------------------------------------------------------------------------
        private void InitBitmap()
        {
            _bmp    = new Bitmap(CANVAS_W, CANVAS_H, System.Drawing.Imaging.PixelFormat.Format32bppArgb);
            _engine = new DrawEngine(_bmp);
            ClearCanvas();
        }

        private void ClearCanvas()
        {
            using (Graphics g = Graphics.FromImage(_bmp))
                g.Clear(Color.Black);
        }

        // -------------------------------------------------------------------------
        // Build the WinForms UI
        // -------------------------------------------------------------------------
        private void BuildUI()
        {
            // ---- Form ----
            Text            = "DRAW Command Demo  —  RetroNick2020 (C# Port)";
            BackColor       = BgDark;
            ForeColor       = PhosGreen;
            Font            = new Font("Consolas", 10f, FontStyle.Regular);
            FormBorderStyle = FormBorderStyle.FixedSingle;
            MaximizeBox     = false;
            StartPosition   = FormStartPosition.CenterScreen;
            ClientSize      = new Size(CANVAS_W + 280, CANVAS_H + 80);

            // =========================================================
            // Title bar strip
            // =========================================================
            _lblTitle = new Label
            {
                Text      = "◈  DRAW COMMAND ENGINE  ◈",
                ForeColor = PhosGreen,
                BackColor = Color.FromArgb(0, 30, 0),
                Font      = new Font("Consolas", 12f, FontStyle.Bold),
                TextAlign = ContentAlignment.MiddleCenter,
                Dock      = DockStyle.Top,
                Height    = 36,
                Padding   = new Padding(0, 4, 0, 0),
            };
            Controls.Add(_lblTitle);

            // =========================================================
            // Canvas  (left side)
            // =========================================================
            _canvasPanel = new Panel
            {
                Location  = new Point(10, 46),
                Size      = new Size(CANVAS_W + 4, CANVAS_H + 4),
                BackColor = Color.Black,
                BorderStyle = BorderStyle.FixedSingle,
            };
            Controls.Add(_canvasPanel);

            _canvas = new PictureBox
            {
                Location = new Point(2, 2),
                Size     = new Size(CANVAS_W, CANVAS_H),
                SizeMode = PictureBoxSizeMode.Normal,
                BackColor = Color.Black,
            };
            _canvas.Paint += Canvas_Paint;
            _canvasPanel.Controls.Add(_canvas);

            // =========================================================
            // Sidebar  (right side)
            // =========================================================
            _sidebar = new Panel
            {
                Location  = new Point(CANVAS_W + 18, 46),
                Size      = new Size(254, CANVAS_H + 4),
                BackColor = PanelBg,
            };
            DrawBorder(_sidebar);
            Controls.Add(_sidebar);

            int sy = 12;

            // ---- Command input label ----
            _lblCmd = MakeLabel("DRAW COMMAND:", 10, sy, 234, 20, PhosGreenDim);
            _lblCmd.Font = new Font("Consolas", 8f);
            _sidebar.Controls.Add(_lblCmd);
            sy += 22;

            // ---- Command TextBox ----
            _cmdInput = new TextBox
            {
                Location    = new Point(10, sy),
                Size        = new Size(234, 24),
                BackColor   = Color.FromArgb(0, 18, 0),
                ForeColor   = PhosGreen,
                Font        = new Font("Consolas", 9f),
                BorderStyle = BorderStyle.FixedSingle,
                Text        = "BM100,100 C14 S8 R50 D50 L50 U50",
            };
            _cmdInput.KeyDown += (s, e) =>
            {
                if (e.KeyCode == Keys.Enter) { ExecuteCmd(); e.SuppressKeyPress = true; }
            };
            _sidebar.Controls.Add(_cmdInput);
            sy += 32;

            // ---- Execute button ----
            _btnExecute = MakeButton("▶  EXECUTE", 10, sy, 234, 32);
            _btnExecute.Click += (s, e) => ExecuteCmd();
            _sidebar.Controls.Add(_btnExecute);
            sy += 40;

            // ---- Demo button ----
            _btnDemo = MakeButton("★  RUN DEMO", 10, sy, 112, 32);
            _btnDemo.Click += (s, e) => { ClearAndReset(); RunDemo(); RefreshCanvas(); };
            _sidebar.Controls.Add(_btnDemo);

            // ---- Clear button ----
            _btnClear = MakeButton("✕  CLEAR", 130, sy, 114, 32);
            _btnClear.BackColor = Color.FromArgb(50, 10, 10);
            _btnClear.ForeColor = Color.FromArgb(255, 80, 80);
            _btnClear.FlatAppearance.BorderColor = Color.FromArgb(120, 30, 30);
            _btnClear.Click += (s, e) => { ClearAndReset(); RefreshCanvas(); };
            _sidebar.Controls.Add(_btnClear);
            sy += 46;

            // ---- Separator ----
            Panel sep = new Panel
            {
                Location  = new Point(10, sy),
                Size      = new Size(234, 1),
                BackColor = BorderCol,
            };
            _sidebar.Controls.Add(sep);
            sy += 10;

            // ---- Pos label ----
            _lblPos = MakeLabel("POS: (0, 0)  COL: 15  SCL: 1.0  ANG: 0°",
                                10, sy, 234, 18, PhosGreenDim);
            _lblPos.Font = new Font("Consolas", 7.5f);
            _sidebar.Controls.Add(_lblPos);
            sy += 26;

            // ---- Log label ----
            _lblLog = MakeLabel("OUTPUT LOG:", 10, sy, 234, 18, PhosGreenDim);
            _lblLog.Font = new Font("Consolas", 8f);
            _sidebar.Controls.Add(_lblLog);
            sy += 20;

            // ---- Log output ----
            _log = new RichTextBox
            {
                Location   = new Point(10, sy),
                Size       = new Size(234, _sidebar.Height - sy - 12),
                BackColor  = Color.FromArgb(0, 10, 0),
                ForeColor  = PhosGreenDim,
                Font       = new Font("Consolas", 8f),
                ReadOnly   = true,
                BorderStyle = BorderStyle.None,
                ScrollBars = RichTextBoxScrollBars.Vertical,
                WordWrap   = true,
            };
            _sidebar.Controls.Add(_log);

            // =========================================================
            // Bottom status strip
            // =========================================================
            Panel statusBar = new Panel
            {
                Location  = new Point(0, CANVAS_H + 52),
                Size      = new Size(ClientSize.Width, 22),
                BackColor = Color.FromArgb(0, 22, 0),
            };
            Label status = new Label
            {
                Text      = "  DJGPP/GRX → C# WinForms  |  Bresenham line engine  |  Scanline flood fill  |  EGA 16-color palette",
                ForeColor = PhosGreenDim,
                Font      = new Font("Consolas", 7.5f),
                AutoSize  = false,
                Dock      = DockStyle.Fill,
                TextAlign = ContentAlignment.MiddleLeft,
            };
            statusBar.Controls.Add(status);
            Controls.Add(statusBar);
        }

        // -------------------------------------------------------------------------
        // Demo - mirrors testod.c exactly
        // -------------------------------------------------------------------------
        private void RunDemo()
        {
            Log("--- DEMO START ---");

            // Yellow square
            _engine.SetPos(0, 0);
            Exec("BM100,100 C14 S8 R50 D50 L50 U50");

            // Red diamond
            _engine.SetPos(320, 150);
            _engine.ColorIndex = 12;
            Exec("E30 F30 G30 H30");

            // Flood fill inside diamond: light red, border dark red
            _engine.SetPos(0, 0);
            Exec("BM370,180 P12,12");

            // Cyan rotated square
            Exec("BM500,200 C11 TA45 S4 R40 D40 L40 U40");

            // Green star using N prefix
            Exec("BM200,350 C10 S8");
            Exec("NU20 ND20 NL20 NR20 NE20 NF20 NG20 NH20");

            Log("--- DEMO END ---");
        }

        // -------------------------------------------------------------------------
        // Execute the command from the text box
        // -------------------------------------------------------------------------
        private void ExecuteCmd()
        {
            string cmd = _cmdInput.Text.Trim();
            if (string.IsNullOrEmpty(cmd)) return;
            Exec(cmd);
            RefreshCanvas();
        }

        private void Exec(string cmd)
        {
            _engine.Execute(cmd);
            UpdateStatusLabel();
            Log("> " + cmd);
        }

        private void ClearAndReset()
        {
            ClearCanvas();
            _engine.SetPos(0, 0);
            _engine.ColorIndex   = 15;
            _engine.Scale        = 1.0;
            _engine.AngleDegrees = 0.0;
            Log("--- CANVAS CLEARED ---");
            UpdateStatusLabel();
        }

        private void UpdateStatusLabel()
        {
            _lblPos.Text = $"POS:({(int)_engine.PosX},{(int)_engine.PosY})  " +
                           $"COL:{_engine.ColorIndex}  " +
                           $"SCL:{_engine.Scale:F2}  " +
                           $"ANG:{_engine.AngleDegrees:F0}°";
        }

        private void Log(string msg)
        {
            _log.AppendText(msg + "\n");
            _log.ScrollToCaret();
        }

        private void RefreshCanvas()
        {
            _canvas.Invalidate();
        }

        // -------------------------------------------------------------------------
        // Paint handler
        // -------------------------------------------------------------------------
        private void Canvas_Paint(object sender, PaintEventArgs e)
        {
            e.Graphics.InterpolationMode = InterpolationMode.NearestNeighbor;
            e.Graphics.PixelOffsetMode   = PixelOffsetMode.Half;
            e.Graphics.DrawImage(_bmp, 0, 0, CANVAS_W, CANVAS_H);
        }

        // -------------------------------------------------------------------------
        // UI factory helpers
        // -------------------------------------------------------------------------
        private Button MakeButton(string text, int x, int y, int w, int h)
        {
            var btn = new Button
            {
                Text      = text,
                Location  = new Point(x, y),
                Size      = new Size(w, h),
                FlatStyle = FlatStyle.Flat,
                BackColor = Color.FromArgb(0, 35, 0),
                ForeColor = PhosGreen,
                Font      = new Font("Consolas", 9f, FontStyle.Bold),
                Cursor    = Cursors.Hand,
            };
            btn.FlatAppearance.BorderColor     = BorderCol;
            btn.FlatAppearance.MouseOverBackColor  = Color.FromArgb(0, 55, 0);
            btn.FlatAppearance.MouseDownBackColor  = Color.FromArgb(0, 80, 10);
            return btn;
        }

        private Label MakeLabel(string text, int x, int y, int w, int h, Color color)
        {
            return new Label
            {
                Text      = text,
                Location  = new Point(x, y),
                Size      = new Size(w, h),
                ForeColor = color,
                BackColor = Color.Transparent,
            };
        }

        private void DrawBorder(Control c)
        {
            c.Paint += (s, e) =>
            {
                ControlPaint.DrawBorder(e.Graphics, c.ClientRectangle,
                    BorderCol, ButtonBorderStyle.Solid);
            };
        }
    }
}
