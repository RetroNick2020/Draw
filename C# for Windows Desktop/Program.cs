// Program.cs
// ============================================================================
// Entry point for the DrawDemo WinForms application.
//
// Target framework: .NET Framework 4.x  OR  .NET 6/7/8 (Windows)
//
// Build with .NET CLI (.NET 6+):
//   dotnet new winforms -n DrawDemo --framework net6.0-windows
//   (replace the generated files with DrawEngine.cs, MainForm.cs, Program.cs)
//   dotnet run
//
// Build with Visual Studio:
//   1. Create a new "Windows Forms App (.NET Framework)" or
//      "Windows Forms App" (.NET 6+) project.
//   2. Add DrawEngine.cs and MainForm.cs to the project.
//   3. Replace Program.cs with this file.
//   4. Build and run.
//
// Build with csc (legacy .NET Framework):
//   csc /target:winexe /out:DrawDemo.exe DrawEngine.cs MainForm.cs Program.cs
//   /reference:System.dll /reference:System.Drawing.dll /reference:System.Windows.Forms.dll
//   /unsafe+
//
// NOTE: DrawEngine.cs uses 'unsafe' pointer code for fast bitmap pixel access
// (matching the Bresenham line and flood fill from the original C code).
// Make sure the project allows unsafe code:
//   .NET CLI:    <AllowUnsafeBlocks>true</AllowUnsafeBlocks> in .csproj
//   Visual Studio: Project Properties -> Build -> Allow unsafe code [check]
//   csc:         /unsafe+ flag (shown above)
// ============================================================================

using System;
using System.Windows.Forms;

namespace DrawDemo
{
    static class Program
    {
        [STAThread]
        static void Main()
        {
            Application.EnableVisualStyles();
            Application.SetCompatibleTextRenderingDefault(false);
            Application.Run(new MainForm());
        }
    }
}
