package com.virtualapplications.play;

import android.content.Context;
import android.graphics.Bitmap;
import android.graphics.BitmapFactory;

import org.apache.commons.io.IOUtils;

import java.io.ByteArrayInputStream;
import java.io.IOException;
import java.io.InputStream;

public class ImageUtils
{
	public static Bitmap getSampledImage(Context context, InputStream inputStream) throws IOException
	{
		if(inputStream.markSupported())
		{
			inputStream.mark(inputStream.available());

			BitmapFactory.Options options = new BitmapFactory.Options();
			options.inJustDecodeBounds = true;
			BitmapFactory.decodeStream(inputStream, null, options);
			inputStream.reset();

			options.inSampleSize = calculateInSampleSize(context, options);
			options.inJustDecodeBounds = false;
			return BitmapFactory.decodeStream(inputStream, null, options);
		}
		else
		{
			ByteArrayInputStream byteArrayInputStream = null;
			try
			{
				byte[] imageArray = IOUtils.toByteArray(inputStream);
				byteArrayInputStream = new ByteArrayInputStream(imageArray);

				return getSampledImage(context, byteArrayInputStream);
			}
			catch(Exception e)
			{
				e.printStackTrace();
			}
			finally
			{
				try
				{
					if(byteArrayInputStream != null)
					{
						byteArrayInputStream.close();
					}
				}
				catch(Exception e)
				{

				}
			}
		}
		return null;
	}

	private static int calculateInSampleSize(Context context, BitmapFactory.Options options)
	{
		final int height = options.outHeight;
		final int width = options.outWidth;

		int reqWidth = (int) context.getResources().getDimension(R.dimen.cover_width);
		int reqHeight = (int) context.getResources().getDimension(R.dimen.cover_height);

		int inSampleSize = 1;

		if(height > reqHeight || width > reqWidth)
		{

			final int halfHeight = height / 2;
			final int halfWidth = width / 2;

			while((halfHeight / inSampleSize) > reqHeight && (halfWidth / inSampleSize) > reqWidth)
			{
				inSampleSize *= 2;
			}
		}

		return inSampleSize;
	}
}
