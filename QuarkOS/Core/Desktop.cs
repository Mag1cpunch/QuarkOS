using System;
using System.Collections.Generic;
using System.Drawing;
using System.Linq;
using System.Text;
using System.Threading.Tasks;
using Cosmos.System;
using QuarkOS.Core.Graphics;

namespace QuarkOS.Core
{
    public class Desktop
    {
        VESACanvas canvas;
        public Desktop(uint width, uint height)
        {
            canvas = new VESACanvas(new Cosmos.System.Graphics.Mode(width, height, Cosmos.System.Graphics.ColorDepth.ColorDepth32));
        }
        public void Run()
        {
            MouseManager.ScreenWidth = canvas.Mode.Width;
            MouseManager.ScreenHeight = canvas.Mode.Height;
            MouseManager.X = canvas.Mode.Width / 2;
            MouseManager.Y = canvas.Mode.Width / 2;
            uint xmargin = 20;
            uint ymargin = 20;

            while (true)
            {
                canvas.Clear(Color.Black);
                canvas.DrawFilledRectangle(Color.White, xmargin, ymargin, );
            }
        }
    }
}
