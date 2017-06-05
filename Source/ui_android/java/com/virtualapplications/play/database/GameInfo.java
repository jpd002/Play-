package com.virtualapplications.play.database;

import java.io.BufferedInputStream;
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
import android.content.ContentResolver;
import android.content.DialogInterface;
import android.content.Intent;
import android.database.Cursor;
import android.graphics.Bitmap;
import android.graphics.BitmapFactory;
import android.os.AsyncTask;
import android.os.Build;
import android.os.StrictMode;
import android.util.Log;
import android.util.LruCache;
import android.util.SparseArray;
import android.view.View;
import android.view.View.OnClickListener;
import android.view.View.OnLongClickListener;
import android.widget.ImageView;
import android.widget.ImageView.ScaleType;
import android.widget.TextView;

import java.util.concurrent.ExecutionException;

import com.virtualapplications.play.GameInfoEditActivity;
import com.virtualapplications.play.GameInfoStruct;
import com.virtualapplications.play.R;
import com.virtualapplications.play.MainActivity;
import com.virtualapplications.play.NativeInterop;
import com.virtualapplications.play.database.SqliteHelper.Games;

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

	public Bitmap getImage(String key, View childview, String boxart, int pos) {
		return getImage(key,childview,boxart, true, pos);
	}
	public Bitmap getImage(final String key, final View childview, String boxart, boolean custom, final int pos) {
		Bitmap cachedImage = getBitmapFromMemCache(key);
		if (cachedImage != null) {
			if (childview != null) {
				ImageView preview = (ImageView) childview.findViewById(R.id.game_icon);
				preview.setImageBitmap(cachedImage);
				((TextView) childview.findViewById(R.id.game_text)).setVisibility(View.GONE);
			}
			return cachedImage;
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
					if (childview != null && Integer.parseInt (((TextView) childview.findViewById(R.id.currentPosition)).getText().toString()) == pos) {
						ImageView preview = (ImageView) childview.findViewById(R.id.game_icon);
						preview.setImageBitmap(bitmap);
						((TextView) childview.findViewById(R.id.game_text)).setVisibility(View.GONE);
					}
				}
			}).execute();


			return null;
		} else {
			new GameImage(childview, boxart, pos).execute(key);
			return null;
		}
	}
	
	public class GameImage extends AsyncTask<String, Integer, Bitmap> {
		
		private View childview;
		private String key;
		private ImageView preview;
		private String boxart;
        private int pos;
		
		public GameImage(View childview, String boxart, int pos) {
			this.childview = childview;
			this.boxart = boxart;
            this.pos = pos;
		}
		
		protected void onPreExecute() {
			if (childview != null) {
				preview = (ImageView) childview.findViewById(R.id.game_icon);
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
			key = params[0];
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
				BufferedInputStream bis = null;
				try {
					URL imageURL = new URL(api);
					URLConnection conn1 = imageURL.openConnection();
					
					im = conn1.getInputStream();
					bis = new BufferedInputStream(im, 512);
					
					BitmapFactory.Options options = new BitmapFactory.Options();
					options.inJustDecodeBounds = true;
					Bitmap bitmap = BitmapFactory.decodeStream(bis, null, options);

					options.inSampleSize = calculateInSampleSize(options);
					options.inJustDecodeBounds = false;
					bis.close();
					im.close();
					conn1 = imageURL.openConnection();
					im = conn1.getInputStream();
					bis = new BufferedInputStream(im, 512);
					bitmap = BitmapFactory.decodeStream(bis, null, options);

					saveImage(key, bitmap);
					return bitmap;
				} catch (IOException e) {
					
				} finally {
					try {
						im.close();
						bis.close();
						im = null;
						bis = null;
					} catch (IOException ex) {}
				}
			}
			return null;
		}
		
		@Override
		protected void onPostExecute(Bitmap image) {
			if (image != null) {
                if (childview != null && Integer.parseInt (((TextView) childview.findViewById(R.id.currentPosition)).getText().toString()) == pos) {
					preview.setImageBitmap(image);
					((TextView) childview.findViewById(R.id.game_text)).setVisibility(View.GONE);
				}
			}
		}
	}
	
	public OnLongClickListener configureLongClick(final String title, final String overview, final GameInfoStruct gameFile) {
		return new OnLongClickListener() {
			public boolean onLongClick(View view) {
				final AlertDialog.Builder builder = new AlertDialog.Builder(mContext);
				builder.setCancelable(true);
				builder.setTitle(title);
				builder.setMessage(overview);
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
								((MainActivity)mContext).launchGame(gameFile);
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
	
	public GameInfoStruct getGameInfo(File game, View childview, GameInfoStruct gameInfoStruct, int pos) {
		String serial = getSerial(game);
		if (serial == null) {
			getImage(game.getName(), childview, null, pos);
			return null;
		}
		String suffix = serial.substring(5, serial.length());
		String gameID = null,  title = null, overview = null, boxart = null;
		ContentResolver cr = mContext.getContentResolver();
		String selection = Games.KEY_SERIAL + "=? OR " + Games.KEY_SERIAL + "=? OR " + Games.KEY_SERIAL + "=? OR "
							+ Games.KEY_SERIAL + "=? OR " + Games.KEY_SERIAL + "=? OR " + Games.KEY_SERIAL + "=?";
		String[] selectionArgs = { serial, "SLUS" + suffix, "SLES" + suffix, "SLPS" + suffix, "SLPM" + suffix, "SCES" + suffix };
		Cursor c = cr.query(Games.GAMES_URI, null, selection, selectionArgs, null);
		if (c != null && c.getCount() > 0) {
			if (c.moveToFirst()) {
				do {
					gameID = c.getString(c.getColumnIndex(Games.KEY_GAMEID));
					title = c.getString(c.getColumnIndex(Games.KEY_TITLE));
					overview = c.getString(c.getColumnIndex(Games.KEY_OVERVIEW));
					boxart = c.getString(c.getColumnIndex(Games.KEY_BOXART));
					if (gameID != null && !gameID.equals("")) {
						break;
					}
				} while (c.moveToNext());
			}
			c.close();
		}
		if (overview != null && boxart != null &&
			!overview.equals("") && !boxart.equals("")) {
			if (gameInfoStruct.isTitleNameEmptyNull()){
				gameInfoStruct.setTitleName(title, mContext);
			}

			if (gameInfoStruct.isDescriptionEmptyNull()){
				gameInfoStruct.setDescription(overview, mContext);
			}
			if (gameInfoStruct.getFrontLink() == null || gameInfoStruct.getFrontLink().isEmpty()){
				gameInfoStruct.setFrontLink(boxart, mContext);
			}
			if (gameInfoStruct.getGameID() == null){
				gameInfoStruct.setGameID(gameID, mContext);
			}
			return gameInfoStruct;
		} else {
			GamesDbAPI gameDatabase = new GamesDbAPI(mContext, gameID, serial, gameInfoStruct, pos);
			gameDatabase.setView(childview);
			gameDatabase.execute(game);
			return null;
		}
	}
	
	public String getSerial(File game) {
		String serial = NativeInterop.getDiskId(game.getPath());
		Log.d("Play!", game.getName() + " [" + serial + "]");
		return serial;
	}
}
