package com.virtualapplications.play.database;

import java.io.File;
import java.io.FileNotFoundException;
import java.io.FileOutputStream;
import java.io.IOException;
import java.io.InputStream;
import java.io.OutputStream;
import java.net.URL;
import java.net.URLConnection;

import android.app.AlertDialog;
import android.content.Context;
import android.content.DialogInterface;
import android.graphics.Bitmap;
import android.graphics.BitmapFactory;
import android.net.ConnectivityManager;
import android.net.NetworkInfo;
import android.os.AsyncTask;
import android.os.Build;
import android.util.LruCache;
import android.view.View;
import android.view.View.OnLongClickListener;

import com.virtualapplications.play.Bootable;
import com.virtualapplications.play.GamesAdapter;
import com.virtualapplications.play.ImageUtils;
import com.virtualapplications.play.MainActivity;
import com.virtualapplications.play.R;

public class GameInfo
{
	public static boolean isNetworkAvailable(Context mContext)
	{
		ConnectivityManager connectivityManager = (ConnectivityManager)mContext
				.getSystemService(Context.CONNECTIVITY_SERVICE);
		NetworkInfo mWifi = connectivityManager.getNetworkInfo(ConnectivityManager.TYPE_WIFI);
		NetworkInfo mMobile = connectivityManager.getNetworkInfo(ConnectivityManager.TYPE_MOBILE);
		NetworkInfo activeNetworkInfo = connectivityManager.getActiveNetworkInfo();
		if(mMobile != null && mWifi != null)
		{
			return mMobile.isAvailable() || mWifi.isAvailable();
		}
		else
		{
			return activeNetworkInfo != null && activeNetworkInfo.isConnected();
		}
	}

	private Context mContext;
	private LruCache<String, Bitmap> mMemoryCache;
	final int maxMemory = (int)(Runtime.getRuntime().maxMemory() / 1024);
	final int cacheSize = maxMemory / 4;

	public GameInfo(Context mContext)
	{
		this.mContext = mContext;
		mMemoryCache = new LruCache<String, Bitmap>(cacheSize)
		{
			@Override
			protected int sizeOf(String key, Bitmap bitmap)
			{
				if(Build.VERSION.SDK_INT >= Build.VERSION_CODES.HONEYCOMB_MR1)
				{
					return bitmap.getByteCount() / 1024;
				}
				else
				{
					return (bitmap.getRowBytes() * bitmap.getHeight()) / 1000;
				}
			}
		};
	}

	public void addBitmapToMemoryCache(String key, Bitmap bitmap)
	{
		if(key != null && !key.equals(null) && bitmap != null)
		{
			if(getBitmapFromMemCache(key) == null)
			{
				mMemoryCache.put(key, bitmap);
			}
		}
	}

	public Bitmap getBitmapFromMemCache(String key)
	{
		return mMemoryCache.get(key);
	}

	public Bitmap removeBitmapFromMemCache(String key)
	{
		return mMemoryCache.remove(key);
	}

	private void setImageViewCover(GamesAdapter.CoverViewHolder viewHolder, Bitmap bitmap, int pos)
	{
		if(viewHolder != null && Integer.parseInt(viewHolder.currentPositionView.getText().toString()) == pos)
		{
			viewHolder.gameImageView.setImageBitmap(bitmap);
			viewHolder.gameTextView.setVisibility(View.GONE);
		}
	}

	private void setDefaultImageViewCover(GamesAdapter.CoverViewHolder viewHolder, int pos)
	{
		if(viewHolder != null && Integer.parseInt(viewHolder.currentPositionView.getText().toString()) == pos)
		{
			viewHolder.gameImageView.setImageResource(R.drawable.boxart);
			viewHolder.gameTextView.setVisibility(View.VISIBLE);
		}
	}

	public void saveImage(String key, Bitmap image)
	{
		saveImage(key, "", image);
	}

	public void saveImage(String key, String custom, Bitmap image)
	{
		addBitmapToMemoryCache(key, image);
		String path = mContext.getExternalFilesDir(null) + "/covers/";
		OutputStream fOut = null;
		File file = new File(path, key + custom + ".jpg"); // the File to save to
		if(!file.getParentFile().exists())
		{
			file.getParentFile().mkdir();
		}
		try
		{
			fOut = new FileOutputStream(file, false);
			image.compress(Bitmap.CompressFormat.JPEG, 85, fOut); // saving the Bitmap to a file compressed as a JPEG with 85% compression rate
			fOut.flush();
		}
		catch(FileNotFoundException e)
		{
			e.printStackTrace();
		}
		catch(IOException e)
		{
			e.printStackTrace();
		}
		finally
		{
			try
			{
				fOut.close();
			}
			catch(IOException ex)
			{
			}
		}
	}

	public void deleteImage(String key, String custom)
	{
		String path = mContext.getExternalFilesDir(null) + "/covers/";
		File file = new File(path, key + custom + ".jpg"); // the File to save to
		if(file.exists())
		{
			file.delete();
		}
	}

	public void setCoverImage(String key, GamesAdapter.CoverViewHolder viewHolder, String boxart, int pos)
	{
		setCoverImage(key, viewHolder, boxart, true, pos);
	}

	public void setCoverImage(final String key, final GamesAdapter.CoverViewHolder viewHolder, String boxart, boolean custom, final int pos)
	{
		Bitmap cachedImage = getBitmapFromMemCache(key);
		if(cachedImage != null)
		{
			setImageViewCover(viewHolder, cachedImage, pos);
			return;
		}
		String path = mContext.getExternalFilesDir(null) + "/covers/";

		File file = new File(path, key + ".jpg");
		if(custom && new File(path, key + "-custom.jpg").exists())
		{
			file = new File(path, key + "-custom.jpg");
		}
		if(file.exists())
		{
			final File finalFile = file;
			(new AsyncTask<Integer, Integer, Bitmap>()
			{
				@Override
				protected Bitmap doInBackground(Integer... integers)
				{
					BitmapFactory.Options options = new BitmapFactory.Options();
					options.inPreferredConfig = Bitmap.Config.ARGB_8888;
					Bitmap bitmap = BitmapFactory.decodeFile(finalFile.getAbsolutePath(), options);
					addBitmapToMemoryCache(key, bitmap);
					return bitmap;
				}

				@Override
				protected void onPostExecute(Bitmap bitmap)
				{
					setImageViewCover(viewHolder, bitmap, pos);
				}
			}).executeOnExecutor(AsyncTask.THREAD_POOL_EXECUTOR);
		}
		else
		{
			new GameImage(viewHolder, boxart, pos).execute(key);
		}
	}

	public class GameImage extends AsyncTask<String, Integer, Bitmap>
	{
		private GamesAdapter.CoverViewHolder viewHolder;
		private String key;
		private String boxart;
		private int pos;

		public GameImage(GamesAdapter.CoverViewHolder viewHolder, String boxart, int pos)
		{
			this.viewHolder = viewHolder;
			this.boxart = boxart;
			this.pos = pos;
		}

		@Override
		protected Bitmap doInBackground(String... params)
		{
			String path = mContext.getExternalFilesDir(null) + "/covers/";
			key = params[0];
			File file = new File(path, key + ".jpg");
			if(!file.exists())
			{
				if(isNetworkAvailable(mContext) && boxart != null)
				{
					String api = null;
					if(boxart.equals("200") || boxart.equals("404"))
					{
						//200 boxart has no link associated with it and was set by the user
						return null;
					}
					else if(!boxart.startsWith("https://cdn.thegamesdb.net/images/original/"))
					{
						api = boxart;
					}
					else
					{
						api = "https://cdn.thegamesdb.net/images/original/" + boxart;
					}
					InputStream im = null;
					try
					{
						URL imageURL = new URL(api);
						URLConnection conn1 = imageURL.openConnection();

						im = conn1.getInputStream();
						Bitmap bitmap = ImageUtils.getSampledImage(mContext, im);

						saveImage(key, bitmap);
						return bitmap;
					}
					catch(IOException e)
					{

					}
					finally
					{
						try
						{
							if(im != null)
							{
								im.close();
							}
							im = null;
						}
						catch(IOException ex)
						{
						}
					}
				}
			}
			return null;
		}

		@Override
		protected void onPostExecute(Bitmap image)
		{
			if(image != null)
			{
				setImageViewCover(viewHolder, image, pos);
			}
			else
			{
				setDefaultImageViewCover(viewHolder, pos);
			}
		}
	}

	public OnLongClickListener configureLongClick(final Bootable gameFile)
	{
		return new OnLongClickListener()
		{
			public boolean onLongClick(View view)
			{
				final AlertDialog.Builder builder = new AlertDialog.Builder(mContext);
				builder.setCancelable(true);
				builder.setTitle(gameFile.title);
				builder.setMessage(gameFile.overview);
				builder.setNegativeButton("Close",
						new DialogInterface.OnClickListener()
						{
							public void onClick(DialogInterface dialog, int which)
							{
								dialog.dismiss();
								return;
							}
						});
				builder.setPositiveButton("Launch",
						new DialogInterface.OnClickListener()
						{
							public void onClick(DialogInterface dialog, int which)
							{
								dialog.dismiss();
								if(mContext instanceof MainActivity)
								{
									((MainActivity)mContext).launchGame(gameFile);
								}
								return;
							}
						});
				builder.create().show();
				return true;
			}
		};
	}
}
