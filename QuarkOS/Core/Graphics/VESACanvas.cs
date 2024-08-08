using Cosmos.Core.Multiboot;
using Cosmos.HAL.Drivers.Video;
using Cosmos.System.Graphics;
using QuarkOS.Core.Drivers.VESA;
using System;
using System.Collections.Generic;
using System.Drawing;
using System.Linq;
using System.Text;
using System.Threading.Tasks;

namespace QuarkOS.Core.Graphics
{
    public class VESACanvas
    {
        static readonly Mode defaultMode = new(1024, 768, ColorDepth.ColorDepth32);
        readonly VESADriver driver;
        Mode mode;

        internal int BytesPerPixel;
        internal int Stride;
        internal int Pitch;


        /// <summary>
        /// Initializes a new instance of the <see cref="VBECanvas"/> class.
        /// </summary>
        public VESACanvas() : this(defaultMode)
        {
        }

        /// <summary>
        /// Initializes a new instance of the <see cref="VBECanvas"/> class.
        /// </summary>
        /// <param name="mode">The display mode to use.</param>
        public unsafe VESACanvas(Mode mode)
        {
            if (Multiboot2.IsVBEAvailable)
            {
                mode = new(Multiboot2.Framebuffer->Width, Multiboot2.Framebuffer->Height, (ColorDepth)Multiboot2.Framebuffer->Bpp);
            }

            ThrowIfModeIsNotValid(mode);

            driver = new VESADriver((ushort)mode.Width, (ushort)mode.Height, (ushort)mode.ColorDepth);
            Mode = mode;
            BytesPerPixel = (int)mode.ColorDepth / 8;
            Stride = (int)mode.ColorDepth / 8;
            Pitch = (int)mode.Width * BytesPerPixel;
        }

        public void Disable()
        {
            driver.DisableDisplay();
        }

        public string Name() => "VESACanvas";

        public Mode Mode
        {
            get => mode;
            set
            {
                mode = value;
                SetMode(mode);
            }
        }

        #region Display
        /// <summary>
        /// Available VBE supported video modes.
        /// <para>
        /// Low res:
        /// <list type="bullet">
        /// <item>320x240x32.</item>
        /// <item>640x480x32.</item>
        /// <item>800x600x32.</item>
        /// <item>1024x768x32.</item>
        /// </list>
        /// </para>
        /// <para>
        /// HD:
        /// <list type="bullet">
        /// <item>1280x720x32.</item>
        /// <item>1280x1024x32.</item>
        /// </list>
        /// </para>
        /// <para>
        /// HDR:
        /// <list type="bullet">
        /// <item>1366x768x32.</item>
        /// <item>1680x1050x32.</item>
        /// </list>
        /// </para>
        /// <para>
        /// HDTV:
        /// <list type="bullet">
        /// <item>1920x1080x32.</item>
        /// <item>1920x1200x32.</item>
        /// </list>
        /// </para>
        /// </summary>
        public List<Mode> AvailableModes { get; } = new()
        {
            new Mode(320, 240, ColorDepth.ColorDepth32),
            new Mode(640, 480, ColorDepth.ColorDepth32),
            new Mode(800, 600, ColorDepth.ColorDepth32),
            new Mode(1024, 768, ColorDepth.ColorDepth32),
            /* The so called HD-Ready resolution */
            new Mode(1280, 720, ColorDepth.ColorDepth32),
            new Mode(1280, 768, ColorDepth.ColorDepth32),
            new Mode(1280, 1024, ColorDepth.ColorDepth32),
            /* A lot of HD-Ready screen uses this instead of 1280x720 */
            new Mode(1366, 768, ColorDepth.ColorDepth32),
            new Mode(1680, 1050, ColorDepth.ColorDepth32),
            /* HDTV resolution */
            new Mode(1920, 1080, ColorDepth.ColorDepth32),
            /* HDTV resolution (16:10 AR) */
            new Mode(1920, 1200, ColorDepth.ColorDepth32),
        };

        public Mode DefaultGraphicsMode => defaultMode;

        /// <summary>
        /// Sets the used display mode, disabling text mode if it is active.
        /// </summary>
        private void SetMode(Mode mode)
        {
            ThrowIfModeIsNotValid(mode);

            ushort xres = (ushort)Mode.Width;
            ushort yres = (ushort)Mode.Height;
            ushort bpp = (ushort)Mode.ColorDepth;

            driver.VBESet(xres, yres, bpp);
        }
        #endregion

        #region Drawing

        public void Clear(int aColor)
        {
            /*
             * TODO this version of Clear() works only when mode.ColorDepth == ColorDepth.ColorDepth32
             * in the other cases you should before convert color and then call the opportune ClearVRAM() overload
             * (the one that takes ushort for ColorDepth.ColorDepth16 and the one that takes byte for ColorDepth.ColorDepth8)
             * For ColorDepth.ColorDepth24 you should mask the Alpha byte.
             */
            switch (mode.ColorDepth)
            {
                case ColorDepth.ColorDepth4:
                    throw new NotImplementedException();
                case ColorDepth.ColorDepth8:
                    throw new NotImplementedException();
                case ColorDepth.ColorDepth16:
                    throw new NotImplementedException();
                case ColorDepth.ColorDepth24:
                    throw new NotImplementedException();
                case ColorDepth.ColorDepth32:
                    driver.ClearVRAM((uint)aColor);
                    break;
                default:
                    throw new NotImplementedException();
            }
        }

        public void Clear(Color aColor)
        {
            /*
             * TODO this version of Clear() works only when mode.ColorDepth == ColorDepth.ColorDepth32
             * in the other cases you should before convert color and then call the opportune ClearVRAM() overload
             * (the one that takes ushort for ColorDepth.ColorDepth16 and the one that takes byte for ColorDepth.ColorDepth8)
             * For ColorDepth.ColorDepth24 you should mask the Alpha byte.
             */
            switch (mode.ColorDepth)
            {
                case ColorDepth.ColorDepth4:
                    throw new NotImplementedException();
                case ColorDepth.ColorDepth8:
                    throw new NotImplementedException();
                case ColorDepth.ColorDepth16:
                    throw new NotImplementedException();
                case ColorDepth.ColorDepth24:
                    throw new NotImplementedException();
                case ColorDepth.ColorDepth32:
                    driver.ClearVRAM((uint)aColor.ToArgb());
                    break;
                default:
                    throw new NotImplementedException();
            }
        }

        /*
         * As DrawPoint() is the basic block of DrawLine() and DrawRect() and in theory of all the future other methods that will
         * be implemented is better to not check the validity of the arguments here or it will repeat the check for any point
         * to be drawn slowing down all.
         */
        public void DrawPoint(Color aColor, int aX, int aY)
        {
            uint offset;

            switch (Mode.ColorDepth)
            {
                case ColorDepth.ColorDepth32:
                    offset = (uint)GetPointOffset(aX, aY);

                    if (aColor.A < 255)
                    {
                        if (aColor.A == 0)
                        {
                            return;
                        }

                        unsafe
                        {
                            int* vramPtr = (int*)driver.buffer.Base;
                            vramPtr += offset / 4; // each int is 4 bytes
                            int currentColor = *vramPtr;
                            int blendedColor = AlphaBlend(aColor.ToArgb(), currentColor, aColor.A);
                            *vramPtr = blendedColor;
                        }
                    }
                    else
                    {
                        unsafe
                        {
                            int* vramPtr = (int*)driver.buffer.Base;
                            vramPtr += offset / 4; // each int is 4 bytes
                            *vramPtr = aColor.ToArgb();
                        }
                    }
                    break;
                case ColorDepth.ColorDepth24:
                    offset = (uint)GetPointOffset(aX, aY);

                    unsafe
                    {
                        byte* vramPtr = (byte*)driver.buffer.Base;
                        vramPtr += offset;
                        vramPtr[0] = aColor.B;
                        vramPtr[1] = aColor.G;
                        vramPtr[2] = aColor.R;
                    }
                    break;
                default:
                    throw new NotImplementedException("Drawing pixels with color depth " + (int)Mode.ColorDepth + " is not yet supported.");
            }
        }

        public void DrawArray(Color[] aColors, int aX, int aY, int aWidth, int aHeight)
        {
            ThrowIfCoordNotValid(aX, aY);
            ThrowIfCoordNotValid(aX + aWidth, aY + aHeight);

            for (int i = 0; i < aX; i++)
            {
                for (int ii = 0; ii < aY; ii++)
                {
                    DrawPoint(aColors[i + (ii * aWidth)], i, ii);
                }
            }
        }

        public void DrawFilledRectangle(Color aColor, int aX, int aY, int aWidth, int aHeight, bool preventOffBoundPixels = true)
        {
            // ClearVRAM clears one uint at a time. So we clear pixelwise not byte wise. That's why we divide by 32 and not 8.
            if (preventOffBoundPixels)
                aWidth = (int)(Math.Min(aWidth, Mode.Width - aX) * (int)Mode.ColorDepth / 32);
            var color = aColor.ToArgb();

            for (int i = aY; i < aY + aHeight; i++)
            {
                driver.ClearVRAM(GetPointOffset(aX, i), aWidth, color);
            }
        }

        public void DrawImage(Image aImage, int aX, int aY, bool preventOffBoundPixels = true)
        {
            var xBitmap = aImage.RawData;
            var xWidth = (int)aImage.Width;
            var xHeight = (int)aImage.Height;
            if (preventOffBoundPixels)
            {
                var maxWidth = Math.Min(xWidth, (int)mode.Width - aX);
                var maxHeight = Math.Min(xHeight, (int)mode.Height - aY);
                int xOffset = aY * (int)Mode.Width + aX;
                for (int i = 0; i < maxHeight; i++)
                {
                    driver.CopyVRAM((i * (int)Mode.Width) + xOffset, xBitmap, i * xWidth, maxWidth);
                }
            }
            else
            {
                int xOffset = aY * xHeight + aX;
                for (int i = 0; i < Mode.Height; i++)
                {
                    driver.CopyVRAM((i * (int)Mode.Width) + xOffset, xBitmap, i * xWidth, xWidth);
                }
            }

        }

        static int[] ScaleImage(Image image, int newWidth, int newHeight)
        {
            int[] pixels = image.RawData;
            int w1 = (int)image.Width;
            int h1 = (int)image.Height;
            int[] temp = new int[newWidth * newHeight];
            int xRatio = (int)((w1 << 16) / newWidth) + 1;
            int yRatio = (int)((h1 << 16) / newHeight) + 1;
            int x2, y2;

            for (int i = 0; i < newHeight; i++)
            {
                for (int j = 0; j < newWidth; j++)
                {
                    x2 = (j * xRatio) >> 16;
                    y2 = (i * yRatio) >> 16;
                    temp[(i * newWidth) + j] = pixels[(y2 * w1) + x2];
                }
            }

            return temp;
        }

        public void DrawImageAlpha(Image image, int x, int y, bool preventOffBoundPixels = true)
        {
            int width = (int)image.Width;
            int height = (int)image.Height;
            int maxWidth = preventOffBoundPixels ? (int)(Math.Min(image.Width, (int)Mode.Width - x)) : (int)image.Width;
            int maxHeight = preventOffBoundPixels ? (int)(Math.Min(image.Height, (int)Mode.Height - y)) : (int)image.Height;

            unsafe
            {
                fixed (int* srcPtr = image.RawData)
                {
                    int* destPtr = (int*)driver.buffer.Base;
                    destPtr += (y * (int)Mode.Width) + x;

                    for (int yi = 0; yi < maxHeight; yi++)
                    {
                        int* srcRowPtr = srcPtr + (yi * width);
                        int* destRowPtr = destPtr + (yi * (int)Mode.Width);

                        for (int xi = 0; xi < maxWidth; xi++)
                        {
                            int srcPixel = srcRowPtr[xi];
                            int alpha = (srcPixel >> 24) & 0xff;

                            if (alpha == 0)
                            {
                                continue;
                            }
                            else if (alpha == 255)
                            {
                                destRowPtr[xi] = srcPixel;
                            }
                            else
                            {
                                int destPixel = destRowPtr[xi];
                                int blendedPixel = AlphaBlend(srcPixel, destPixel, alpha);
                                destRowPtr[xi] = blendedPixel;
                            }
                        }
                    }
                }
            }
        }

        #endregion

        public void Display()
        {
            driver.Swap();
        }

        protected bool CheckIfModeIsValid(Mode mode)
        {
            foreach (var elem in AvailableModes)
            {
                if (elem == mode)
                {
                    return true; // All OK mode does exists in availableModes
                }
            }

            return false;
        }

        protected void ThrowIfModeIsNotValid(Mode mode)
        {
            if (CheckIfModeIsValid(mode))
            {
                return;
            }

            throw new ArgumentOutOfRangeException(nameof(mode), $"Mode {mode} is not supported by this driver");
        }

        protected void ThrowIfCoordNotValid(int x, int y)
        {
            if (x < 0 || x >= Mode.Width)
            {
                throw new ArgumentOutOfRangeException(nameof(x), $"X coordinate ({x}) is not between 0 and {Mode.Width}");
            }

            if (y < 0 || y >= Mode.Height)
            {
                throw new ArgumentOutOfRangeException(nameof(y), $"Y coordinate ({y}) is not between 0 and {Mode.Height}");
            }
        }

        public static int AlphaBlend(int src, int dest, int alpha)
        {
            int invAlpha = 255 - alpha;

            int srcR = (src >> 16) & 0xff;
            int srcG = (src >> 8) & 0xff;
            int srcB = src & 0xff;

            int destR = (dest >> 16) & 0xff;
            int destG = (dest >> 8) & 0xff;
            int destB = dest & 0xff;

            int blendedR = ((srcR * alpha) + (destR * invAlpha)) >> 8;
            int blendedG = ((srcG * alpha) + (destG * invAlpha)) >> 8;
            int blendedB = ((srcB * alpha) + (destB * invAlpha)) >> 8;

            return (blendedR << 16) | (blendedG << 8) | blendedB | (0xff << 24);
        }

        internal int GetPointOffset(int x, int y)
        {
            return (x * Stride) + (y * Pitch);
        }

        #region Reading

        public Color GetPointColor(int aX, int aY)
        {
            uint offset = (uint)GetPointOffset(aX, aY);
            return Color.FromArgb((int)driver.GetVRAM(offset));
        }

        #endregion
    }
}
