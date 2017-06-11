package com.virtualapplications.play.database;

import java.io.ByteArrayInputStream;
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
import android.content.Intent;
import android.graphics.Bitmap;
import android.graphics.BitmapFactory;
import android.os.AsyncTask;
import android.os.Build;
import android.util.LruCache;
import android.view.View;
import android.view.View.OnLongClickListener;
import android.widget.ImageView;

import com.virtualapplications.play.GameInfoEditActivity;
import com.virtualapplications.play.GameInfoStruct;
import com.virtualapplications.play.GamesAdapter;
import com.virtualapplications.play.MainActivity;

import org.apache.commons.io.IOUtils;

import static com.virtualapplications.play.MainActivity.launchGame;

public class GameInfo {
	
	private Context mContext;
	private LruCache<String, Bitmap> mMemoryCache;
	final int maxMemory = (int) (Runtime.getRuntime().maxMemory() / 1024);
	final int cacheSize = maxMemory / 4;
	
	public GameInfo (Context mContext) {
		this.mContext = mContext;
		mMemoryCache = new LruCache<String, Bitmap>(cacheSize) {
			@Override
			protected int sizeOf(String key, Bitmap bitmap) {
                if (Build.VERSION.SDK_INT >= Build.VERSION_CODES.HONEYCOMB_MR1) {
					return bitmap.getByteCount() / 1024;
                } else {
                	return (bitmap.getRowBytes() * bitmap.getHeight()) / 1000;
                }
			}
		};
	}
    
	public void addBitmapToMemoryCache(String key, Bitmap bitmap) {
		if (key != null && !key.equals(null) && bitmap != null) {
			if (getBitmapFromMemCache(key) == null) {
                mMemoryCache.put(key, bitmap);
			}
		}
	}

	public Bitmap getBitmapFromMemCache(String key) {
		return mMemoryCache.get(key);
	}

	public Bitmap removeBitmapFromMemCache(String key) {
		return mMemoryCache.remove(key);
	}

	private void setImageViewCover(GamesAdapter.CoverViewHolder viewHolder, Bitmap bitmap, int pos)
	{
		if (viewHolder != null && Integer.parseInt (viewHolder.currentPositionView.getText().toString()) == pos) {
			viewHolder.gameImageView.setImageBitmap(bitmap);
			viewHolder.gameTextView.setVisibility(View.GONE);
		}
	}

	public void saveImage(String key, Bitmap image) {
		saveImage(key, "", image);
	}

	public void saveImage(String key, String custom, Bitmap image) {
		addBitmapToMemoryCache(key, image);
		String path = mContext.getExternalFilesDir(null) + "/covers/";
		OutputStream fOut = null;
		File file = new File(path, key + custom + ".jpg"); // the File to save to
		if (!file.getParentFile().exists()) {
			file.getParentFile().mkdir();
		}
		try {
			fOut = new FileOutputStream(file, false);
			image.compress(Bitmap.CompressFormat.JPEG, 85, fOut); // saving the Bitmap to a file compressed as a JPEG with 85% compression rate
			fOut.flush();
		} catch (FileNotFoundException e) {
			e.printStackTrace();
		} catch (IOException e) {
			e.printStackTrace();
		} finally {
			try {
				fOut.close();
			} catch (IOException ex) {}
		}
	}

	public void deleteImage(String key, String custom) {
		String path = mContext.getExternalFilesDir(null) + "/covers/";
		File file = new File(path, key + custom + ".jpg"); // the File to save to
		if (file.exists()) {
			file.delete();
		}
	}

	public void setCoverImage(String key, GamesAdapter.CoverViewHolder viewHolder, String boxart, int pos) {
		setCoverImage(key,viewHolder,boxart, true, pos);
	}
	public void setCoverImage(final String key, final GamesAdapter.CoverViewHolder viewHolder, String boxart, boolean custom, final int pos) {
		Bitmap cachedImage = getBitmapFromMemCache(key);
		if (cachedImage != null) {
			setImageViewCover(viewHolder, cachedImage, pos);
			return;
		}
		String path = mContext.getExternalFilesDir(null) + "/covers/";

		File file = new File(path, key + ".jpg");
		if (custom && new File(path, key + "-custom.jpg").exists()) file = new File(path, key + "-custom.jpg");
		if(file.exists())
		{
			final File finalFile = file;
			(new AsyncTask<Integer, Integer, Bitmap>(){
				@Override
				protected Bitmap doInBackground(Integer... integers) {
					BitmapFactory.Options options = new BitmapFactory.Options();
					options.inPreferredConfig = Bitmap.Config.ARGB_8888;
					Bitmap bitmap = BitmapFactory.decodeFile(finalFile.getAbsolutePath(), options);
					addBitmapToMemoryCache(key, bitmap);
					return bitmap;
				}
				@Override
				protected void onPostExecute(Bitmap bitmap) {
					setImageViewCover(viewHolder, bitmap, pos);

				}
			}).executeOnExecutor(AsyncTask.THREAD_POOL_EXECUTOR);
		} else {
			new GameImage(viewHolder, boxart, pos).execute(key);
		}
	}
	
	public class GameImage extends AsyncTask<String, Integer, Bitmap> {
		
		private GamesAdapter.CoverViewHolder viewHolder;
		private String key;
		private ImageView preview;
		private String boxart;
        private int pos;
		
		public GameImage(GamesAdapter.CoverViewHolder viewHolder, String boxart, int pos) {
			this.viewHolder = viewHolder;
			this.boxart = boxart;
            this.pos = pos;
		}
		
		protected void onPreExecute() {
			if (viewHolder != null) {
				preview = viewHolder.gameImageView;
			}
		}
		
		private int calculateInSampleSize(BitmapFactory.Options options) {
			final int height = options.outHeight;
			final int width = options.outWidth;
			int reqHeight = 420;
			int reqWidth = 360;
			if (preview != null) {
				reqHeight = preview.getMeasuredHeight();
				reqWidth = preview.getMeasuredWidth();
			}
			// TODO: Find a calculated width and height without ImageView
			int inSampleSize = 1;
			
			if (height > reqHeight || width > reqWidth) {
				
				final int halfHeight = height / 2;
				final int halfWidth = width / 2;
				
				while ((halfHeight / inSampleSize) > reqHeight
					   && (halfWidth / inSampleSize) > reqWidth) {
					inSampleSize *= 2;
				}
			}
			
			return inSampleSize;
		}
		
		@Override
		protected Bitmap doInBackground(String... params) {
			String path = mContext.getExternalFilesDir(null) + "/covers/";
			key = params[0];
			File file = new File(path, key + ".jpg");
			if(!file.exists()) {
				if (GamesDbAPI.isNetworkAvailable(mContext) && boxart != null) {
					String api = null;
					if (!boxart.startsWith("boxart/original/front/")) {
						api = boxart;
					} else if (boxart.equals("200")) {
						//200 boxart has no link associated with it and was set by the user
						return null;
					} else {
						api = "http://thegamesdb.net/banners/" + boxart;
					}
					InputStream im = null;
					ByteArrayInputStream byteArrayInputStream = null;
					try {
						URL imageURL = new URL(api);
						URLConnection conn1 = imageURL.openConnection();

						im = conn1.getInputStream();
						byte[] imageArray = IOUtils.toByteArray(im);

						byteArrayInputStream = new ByteArrayInputStream(imageArray);
						byteArrayInputStream.mark(byteArrayInputStream.available());

						BitmapFactory.Options options = new BitmapFactory.Options();
						options.inJustDecodeBounds = true;
						BitmapFactory.decodeStream(byteArrayInputStream, null, options);
						byteArrayInputStream.reset();

						options.inSampleSize = calculateInSampleSize(options);
						options.inJustDecodeBounds = false;
						Bitmap bitmap = BitmapFactory.decodeStream(byteArrayInputStream, null, options);

						saveImage(key, bitmap);
						return bitmap;
					} catch (IOException e) {

					} finally {
						try {
							im.close();
							byteArrayInputStream.close();
							byteArrayInputStream = null;
							im = null;
						} catch (IOException ex) {}
					}
				}
			}
			return null;
		}
		
		@Override
		protected void onPostExecute(Bitmap image) {
			if (image != null) {
				setImageViewCover(viewHolder, image, pos);
			}
		}
	}
	
	public OnLongClickListener configureLongClick(final GameInfoStruct gameFile) {
		return new OnLongClickListener() {
			public boolean onLongClick(View view) {
				final AlertDialog.Builder builder = new AlertDialog.Builder(mContext);
				builder.setCancelable(true);
				builder.setTitle(gameFile.getTitleName());
				builder.setMessage(gameFile.getDescription());
				builder.setNegativeButton("Close",
						new DialogInterface.OnClickListener() {
							public void onClick(DialogInterface dialog, int which) {
								dialog.dismiss();
								return;
							}
						});
				builder.setPositiveButton("Launch",
						new DialogInterface.OnClickListener() {
							public void onClick(DialogInterface dialog, int which) {
								dialog.dismiss();
								launchGame(gameFile, mContext);
								return;
							}
						});
				builder.setNeutralButton("Edit",
						new DialogInterface.OnClickListener() {
							public void onClick(DialogInterface dialog, int which) {
								dialog.dismiss();
								Intent intent = new Intent(mContext, GameInfoEditActivity.class);
								intent.putExtra("title",gameFile.getTitleName());
								intent.putExtra("overview",gameFile.getDescription());
								intent.putExtra("cover",gameFile.getFrontLink());
								intent.putExtra("gameid",gameFile.getGameID());
								intent.putExtra("indexid",gameFile.getIndexID());
								((MainActivity)mContext).startActivityForResult(intent, 1);
								return;
							}
						});
				builder.create().show();
				return true;
			}
		};
	}
	
	public void loadGameInfo(GamesAdapter.CoverViewHolder viewHolder, GameInfoStruct gameInfoStruct, int pos)
	{
		GamesDbAPI gameDatabase = new GamesDbAPI(mContext, gameInfoStruct, pos);
		gameDatabase.setView(viewHolder);
		gameDatabase.execute();
	}
}
