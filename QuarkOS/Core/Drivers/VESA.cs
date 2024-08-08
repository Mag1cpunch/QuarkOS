using Cosmos.Core;
using Cosmos.Core.Multiboot;
using Cosmos.HAL;
using QuarkOS.Core.Utils;
using System;
using System.Collections.Generic;
using System.Linq;
using System.Text;
using System.Threading.Tasks;

namespace QuarkOS.Core.Drivers.VESA
{
    public class VESADriver
    {
        public const short VBEIndex = 0x01CE;
        public const short VBEData = 0x01CF;

        public MemoryBlock buffer;
        protected readonly ManagedMemoryBlock lastbuffer;

        public enum RegisterIndex
        {
            DisplayID = 0x00,
            DisplayXResolution,
            DisplayYResolution,
            DisplayBPP,
            DisplayEnable,
            DisplayBankMode,
            DisplayVirtualWidth,
            DisplayVirtualHeight,
            DisplayXOffset,
            DisplayYOffset
        };

        [Flags]
        public enum EnableValues
        {
            Disabled = 0x00,
            Enabled,
            UseLinearFrameBuffer = 0x40,
            NoClearMemory = 0x80,
        };

        public unsafe VESADriver(ushort xres, ushort yres, ushort bpp)
        {
            PCIDevice videocard;

            if (Multiboot2.IsVBEAvailable) //VBE VESA Enabled Mulitboot Parsing
            {
                Cosmos.Core.Global.debugger.SendInternal($"Creating VESA driver with Mode {xres}*{yres}@{bpp}");
                buffer = new MemoryBlock((uint)Multiboot2.Framebuffer->Address, (uint)xres * yres * (uint)(bpp / 8));
                lastbuffer = new ManagedMemoryBlock((uint)xres * yres * (uint)(bpp / 8));
            }
            else if (ISAModeAvailable()) //Bochs Graphics Adaptor ISA Mode
            {
                Cosmos.Core.Global.debugger.SendInternal($"Creating VBE BGA driver with Mode {xres}*{yres}@{bpp}.");
                buffer = new MemoryBlock(0xE0000000, 1920 * 1200 * 4);
                lastbuffer = new ManagedMemoryBlock(1920 * 1200 * 4);
                VBESet(xres, yres, bpp);
            }
            else if ((videocard = PCI.GetDevice(VendorID.VirtualBox, DeviceID.VBVGA)) != null || //VirtualBox Video Adapter PCI Mode
            (videocard = PCI.GetDevice(VendorID.Bochs, DeviceID.BGA)) != null) // Bochs Graphics Adaptor PCI Mode
            {
                Cosmos.Core.Global.debugger.SendInternal($"Creating VBE BGA driver with Mode {xres}*{yres}@{bpp}. Framebuffer address=" + videocard.BAR0);

                buffer = new MemoryBlock(videocard.BAR0, 1920 * 1200 * 4);
                lastbuffer = new ManagedMemoryBlock(1920 * 1200 * 4);
                VBESet(xres, yres, bpp);
            }
            else
            {
                throw new Exception("No supported VBE device found.");
            }
        }

        public static void VBEWrite(RegisterIndex index, ushort value)
        {
            IOPort.Write16(VBEIndex, (ushort)index);
            IOPort.Write16(VBEData, value);
        }

        public static ushort VBERead(RegisterIndex index)
        {
            IOPort.Write16(VBEIndex, (ushort)index);
            return IOPort.Read16(VBEData);
        }

        public static bool ISAModeAvailable()
        {
            return VBERead(RegisterIndex.DisplayID) == 0xB0C5;
        }

        public void DisableDisplay()
        {
            Cosmos.Core.Global.debugger.SendInternal($"Disabling VBE display");
            VBEWrite(RegisterIndex.DisplayEnable, (ushort)EnableValues.Disabled);
        }

        public void SetXResolution(ushort xres)
        {
            Cosmos.Core.Global.debugger.SendInternal($"VBE Setting X resolution to {xres}");
            VBEWrite(RegisterIndex.DisplayXResolution, xres);
        }

        public void SetYResolution(ushort yres)
        {
            Cosmos.Core.Global.debugger.SendInternal($"VBE Setting Y resolution to {yres}");
            VBEWrite(RegisterIndex.DisplayYResolution, yres);
        }

        public void SetDisplayBPP(ushort bpp)
        {
            Cosmos.Core.Global.debugger.SendInternal($"VBE Setting BPP to {bpp}");
            VBEWrite(RegisterIndex.DisplayBPP, bpp);
        }

        public void EnableDisplay(EnableValues EnableFlags)
        {
            VBEWrite(RegisterIndex.DisplayEnable, (ushort)EnableFlags);
        }

        public void VBESet(ushort xres, ushort yres, ushort bpp, bool clear = false)
        {
            DisableDisplay();
            SetXResolution(xres);
            SetYResolution(yres);
            SetDisplayBPP(bpp);
            if (clear)
            {
                EnableDisplay(EnableValues.Enabled | EnableValues.UseLinearFrameBuffer);

            }
            else
            {
                EnableDisplay(EnableValues.Enabled | EnableValues.UseLinearFrameBuffer | EnableValues.NoClearMemory);
            }
        }

        public void VBESet(ushort mode, bool clear = false)
        {
            DisableDisplay();

            IOPort.Write16(VBEIndex, 0x4F02); // Set mode function
            IOPort.Write16(VBEData, mode);

            if (clear) { EnableDisplay(EnableValues.Enabled | EnableValues.UseLinearFrameBuffer); return; }

            EnableDisplay(EnableValues.Enabled | EnableValues.UseLinearFrameBuffer | EnableValues.NoClearMemory);
        }

        public void SetVRAM(uint index, byte value)
        {
            lastbuffer[index] = value;
        }

        public void SetVRAM(uint index, ushort value)
        {
            lastbuffer[index] = (byte)((value >> 8) & 0xFF);
            lastbuffer[index + 1] = (byte)((value >> 0) & 0xFF);
        }

        public void SetVRAM(uint index, uint value)
        {
            lastbuffer[index] = (byte)((value >> 24) & 0xFF);
            lastbuffer[index + 1] = (byte)((value >> 16) & 0xFF);
            lastbuffer[index + 2] = (byte)((value >> 8) & 0xFF);
            lastbuffer[index + 3] = (byte)((value >> 0) & 0xFF);
        }

        public void SetVRAM(uint index, ulong value)
        {
            lastbuffer[index] = (byte)((value >> 56) & 0xFF);
            lastbuffer[index + 1] = (byte)((value >> 48) & 0xFF);
            lastbuffer[index + 2] = (byte)((value >> 40) & 0xFF);
            lastbuffer[index + 3] = (byte)((value >> 32) & 0xFF);
            lastbuffer[index + 4] = (byte)((value >> 24) & 0xFF);
            lastbuffer[index + 5] = (byte)((value >> 16) & 0xFF);
            lastbuffer[index + 6] = (byte)((value >> 8) & 0xFF);
            lastbuffer[index + 7] = (byte)((value >> 0) & 0xFF);
        }

        public void SetVRAM(uint index, byte[] values)
        {
            for (int i = 0; i < values.Length; i++)
            {
                SetVRAM((uint)(index + i), values[i]);
            }
        }

        public uint GetVRAM(uint index)
        {
            int pixel = (lastbuffer[index + 3] << 24) | (lastbuffer[index + 2] << 16) | (lastbuffer[index + 1] << 8) | lastbuffer[index];
            return (uint)pixel;
        }

        public void ClearVRAM(uint value)
        {
            lastbuffer.Fill(value);
        }

        public void ClearVRAM(int aStart, int aCount, int value)
        {
            lastbuffer.Fill(aStart, aCount, value);
        }

        public void CopyVRAM(int aStart, int[] aData, int aIndex, int aCount)
        {
            lastbuffer.Copy(aStart, aData, aIndex, aCount);
        }

        public void CopyVRAM(int aStart, byte[] aData, int aIndex, int aCount)
        {
            lastbuffer.Copy(aStart, aData, aIndex, aCount);
        }

        public void Swap()
        {
            buffer.Copy(lastbuffer);
        }
    }
}
