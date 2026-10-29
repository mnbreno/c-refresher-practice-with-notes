using System;
using System.Drawing;
using System.Drawing.Drawing2D;
using System.Drawing.Text;
using System.IO;
using System.Text;

public static class CalculatorIconBuilder
{
    private static GraphicsPath RoundedRectangle(float x, float y, float width,
                                                  float height, float radius)
    {
        var path = new GraphicsPath();
        float diameter = radius * 2;
        path.AddArc(x, y, diameter, diameter, 180, 90);
        path.AddArc(x + width - diameter, y, diameter, diameter, 270, 90);
        path.AddArc(x + width - diameter, y + height - diameter,
                    diameter, diameter, 0, 90);
        path.AddArc(x, y + height - diameter, diameter, diameter, 90, 90);
        path.CloseFigure();
        return path;
    }

    public static byte[] RenderPng(int size)
    {
        using (var bitmap = new Bitmap(size, size,
                   System.Drawing.Imaging.PixelFormat.Format32bppArgb))
        using (var graphics = Graphics.FromImage(bitmap))
        using (var output = new MemoryStream())
        {
            graphics.SmoothingMode = SmoothingMode.AntiAlias;
            graphics.InterpolationMode = InterpolationMode.HighQualityBicubic;
            graphics.PixelOffsetMode = PixelOffsetMode.HighQuality;
            graphics.TextRenderingHint = TextRenderingHint.AntiAliasGridFit;
            graphics.Clear(Color.Transparent);
            graphics.ScaleTransform(size / 128f, size / 128f);

            using (var tile = RoundedRectangle(5, 5, 118, 118, 24))
            using (var brush = new SolidBrush(Color.FromArgb(24, 45, 59)))
                graphics.FillPath(brush, tile);

            using (var panel = RoundedRectangle(18, 17, 92, 38, 12))
            using (var brush = new LinearGradientBrush(
                       new Rectangle(18, 17, 92, 38),
                       Color.FromArgb(66, 197, 171),
                       Color.FromArgb(31, 139, 151), 0f))
                graphics.FillPath(brush, panel);

            using (var font = new Font("Segoe UI", 18, FontStyle.Bold,
                                       GraphicsUnit.Pixel))
            using (var brush = new SolidBrush(Color.FromArgb(16, 43, 48)))
                graphics.DrawString("42", font, brush, 72, 22);

            Color[] keyColors = {
                Color.FromArgb(226, 235, 230), Color.FromArgb(226, 235, 230),
                Color.FromArgb(240, 151, 92), Color.FromArgb(226, 235, 230),
                Color.FromArgb(226, 235, 230), Color.FromArgb(240, 151, 92),
                Color.FromArgb(226, 235, 230), Color.FromArgb(226, 235, 230),
                Color.FromArgb(240, 151, 92)
            };
            int index = 0;
            for (int row = 0; row < 3; row++)
            {
                for (int column = 0; column < 3; column++)
                {
                    var key = new Rectangle(19 + column * 31,
                                            62 + row * 20, 23, 14);
                    using (var brush = new SolidBrush(keyColors[index++]))
                        graphics.FillRectangle(brush, key);
                }
            }

            using (var font = new Font("Segoe UI", 8, FontStyle.Bold,
                                       GraphicsUnit.Pixel))
            using (var brush = new SolidBrush(Color.FromArgb(24, 45, 59)))
                graphics.DrawString("=", font, brush, 91, 100);

            bitmap.Save(output, System.Drawing.Imaging.ImageFormat.Png);
            return output.ToArray();
        }
    }

    public static void WriteIco(string path)
    {
        int[] sizes = { 16, 24, 32, 48, 64, 128, 256 };
        byte[][] images = new byte[sizes.Length][];
        for (int i = 0; i < sizes.Length; i++)
            images[i] = RenderPng(sizes[i]);

        using (var stream = File.Create(path))
        using (var writer = new BinaryWriter(stream))
        {
            writer.Write((ushort)0);
            writer.Write((ushort)1);
            writer.Write((ushort)sizes.Length);
            int offset = 6 + 16 * sizes.Length;
            for (int i = 0; i < sizes.Length; i++)
            {
                byte dimension = (byte)(sizes[i] == 256 ? 0 : sizes[i]);
                writer.Write(dimension);
                writer.Write(dimension);
                writer.Write((byte)0);
                writer.Write((byte)0);
                writer.Write((ushort)1);
                writer.Write((ushort)32);
                writer.Write((uint)images[i].Length);
                writer.Write((uint)offset);
                offset += images[i].Length;
            }
            for (int i = 0; i < images.Length; i++)
                writer.Write(images[i]);
        }
    }

    private static void WriteBigEndian(BinaryWriter writer, uint value)
    {
        writer.Write((byte)(value >> 24));
        writer.Write((byte)(value >> 16));
        writer.Write((byte)(value >> 8));
        writer.Write((byte)value);
    }

    public static void WriteIcns(string path)
    {
        int[] sizes = { 16, 32, 64, 128, 256, 512, 1024 };
        string[] types = { "icp4", "icp5", "icp6", "ic07", "ic08", "ic09", "ic10" };
        byte[][] images = new byte[sizes.Length][];
        uint totalLength = 8;
        for (int i = 0; i < sizes.Length; i++)
        {
            images[i] = RenderPng(sizes[i]);
            totalLength += (uint)(8 + images[i].Length);
        }

        using (var stream = File.Create(path))
        using (var writer = new BinaryWriter(stream))
        {
            writer.Write(Encoding.ASCII.GetBytes("icns"));
            WriteBigEndian(writer, totalLength);
            for (int i = 0; i < images.Length; i++)
            {
                writer.Write(Encoding.ASCII.GetBytes(types[i]));
                WriteBigEndian(writer, (uint)(8 + images[i].Length));
                writer.Write(images[i]);
            }
        }
    }
}