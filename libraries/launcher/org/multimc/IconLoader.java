package org.multimc;

import javax.imageio.ImageIO;
import java.awt.*;
import java.awt.image.BufferedImage;
import java.io.File;
import java.io.IOException;
import java.nio.ByteBuffer;

/*****************************************************************************
 * A convenience class for loading icons from images.
 *
 * Icons loaded from this class are formatted to fit within the required
 * dimension (16x16, 32x32, or 128x128). If the source image is larger than the
 * target dimension, it is shrunk down to the minimum size that will fit. If it
 * is smaller, then it is only scaled up if the new scale can be a per-pixel
 * linear scale (i.e., x2, x3, x4, etc). In both cases, the image's width/height
 * ratio is kept the same as the source image.
 *
 * @author Chris Molini
 *****************************************************************************/
public class IconLoader
{
	/*************************************************************************
	 * Loads an icon in ByteBuffer form.
	 *
	 * @param filepath
	 *            The location of the Image to use as an icon.
	 *
	 * @return An array of ByteBuffers containing the pixel data for the icon in
	 *         various sizes (as recommended by the OS).
	 *************************************************************************/
	public static ByteBuffer[] load(String filepath)
	{
		BufferedImage image;
		try {
			image = ImageIO.read ( new File( filepath ) );
		} catch ( IOException e ) {
			e.printStackTrace();
			return new ByteBuffer[0];
		}
		ByteBuffer[] buffers;
		buffers = new ByteBuffer[1];
		buffers[0] = loadInstance(image, 128);
		return buffers;
	}

	/*************************************************************************
	 * Copies the supplied image into a square icon at the indicated size.
	 *
	 * @param image
	 *            The image to place onto the icon.
	 * @param dimension
	 *            The desired size of the icon.
	 *
	 * @return A ByteBuffer of pixel data at the indicated size.
	 *************************************************************************/
	private static ByteBuffer loadInstance(BufferedImage image, int dimension)
	{
		BufferedImage scaledIcon = new BufferedImage(dimension, dimension,
				BufferedImage.TYPE_INT_ARGB_PRE);
		Graphics2D g = scaledIcon.createGraphics();
		double ratio = getIconRatio(image, scaledIcon);
		double width = image.getWidth() * ratio;
		double height = image.getHeight() * ratio;
		g.drawImage(image, (int) ((scaledIcon.getWidth() - width) / 2),
				(int) ((scaledIcon.getHeight() - height) / 2), (int) (width),
				(int) (height), null);
		g.dispose();

		return convertToByteBuffer(scaledIcon);
	}

	/*************************************************************************
	 * Gets the width/height ratio of the icon. This is meant to simplify
	 * scaling the icon to a new dimension.
	 *
	 * @param src
	 *            The base image that will be placed onto the icon.
	 * @param icon
	 *            The icon that will have the image placed on it.
	 *
	 * @return The amount to scale the source image to fit it onto the icon
	 *         appropriately.
	 *************************************************************************/
	private static double getIconRatio(BufferedImage src, BufferedImage icon)
	{
		double ratio = 1;
		if (src.getWidth() > icon.getWidth())
			ratio = (double) (icon.getWidth()) / src.getWidth();
		else
			ratio = (int) (icon.getWidth() / src.getWidth());
		if (src.getHeight() > icon.getHeight())
		{
			double r2 = (double) (icon.getHeight()) / src.getHeight();
			if (r2 < ratio)
				ratio = r2;
		}
		else
		{
			double r2 = (int) (icon.getHeight() / src.getHeight());
			if (r2 < ratio)
				ratio = r2;
		}
		return ratio;
	}

	/*************************************************************************
	 * Converts a BufferedImage into a ByteBuffer of pixel data.
	 *
	 * @param image
	 *            The image to convert.
	 *
	 * @return A ByteBuffer that contains the pixel data of the supplied image.
	 *************************************************************************/
	public static ByteBuffer convertToByteBuffer(BufferedImage image)
	{
		byte[] buffer = new byte[image.getWidth() * image.getHeight() * 4];
		int counter = 0;
		for (int i = 0; i < image.getHeight(); i++)
			for (int j = 0; j < image.getWidth(); j++)
			{
				int colorSpace = image.getRGB(j, i);
				buffer[counter + 0] = (byte) ((colorSpace << 8) >> 24);
				buffer[counter + 1] = (byte) ((colorSpace << 16) >> 24);
				buffer[counter + 2] = (byte) ((colorSpace << 24) >> 24);
				buffer[counter + 3] = (byte) (colorSpace >> 24);
				counter += 4;
			}
		return ByteBuffer.wrap(buffer);
	}
}