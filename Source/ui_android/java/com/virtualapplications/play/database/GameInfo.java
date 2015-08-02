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
import android.database.Cursor;
import android.graphics.Bitmap;
import android.graphics.BitmapFactory;
import android.os.AsyncTask;
import android.os.Build;
import android.os.StrictMode;
import android.util.Log;
import android.util.SparseArray;
import android.view.View;
import android.view.View.OnClickListener;
import android.view.View.OnLongClickListener;
import android.widget.ImageView;
import android.widget.ImageView.ScaleType;
import android.widget.TextView;

import java.util.concurrent.ExecutionException;

import com.virtualapplications.play.R;
import com.virtualapplications.play.MainActivity;
import com.virtualapplications.play.NativeInterop;
import com.virtualapplications.play.database.SqliteHelper.Games;

public class GameInfo {
	
	private Context mContext;
	
	public GameInfo (Context mContext) {
		this.mContext = mContext;
	}
	
	public void saveImage(String key, Bitmap image) {
		String path = mContext.getExternalFilesDir(null) + "/covers/";
		OutputStream fOut = null;
		File file = new File(path, key + ".jpg"); // the File to save to
		if (!file.getParentFile().exists()) {
			file.getParentFile().mkdir();
		}
		try {
			fOut = new FileOutputStream(file, false);
			image.compress(Bitmap.CompressFormat.JPEG, 85, fOut); // saving the Bitmap to a file compressed as a JPEG with 85% compression rate
			fOut.flush();
			fOut.close(); // do not forget to close the stream
		} catch (FileNotFoundException e) {
			e.printStackTrace();
		} catch (IOException e) {
			e.printStackTrace();
		}
		
	}
	
	public Bitmap getImage(String key, View childview, String boxart) {
		String path = mContext.getExternalFilesDir(null) + "/covers/";
		
		File file = new File(path, key + ".jpg");
		if(file.exists())
		{
			BitmapFactory.Options options = new BitmapFactory.Options();
			options.inPreferredConfig = Bitmap.Config.ARGB_8888;
			Bitmap bitmap = BitmapFactory.decodeFile(file.getAbsolutePath(), options);
			if (childview != null) {
				ImageView preview = (ImageView) childview.findViewById(R.id.game_icon);
				preview.setImageBitmap(bitmap);
				preview.setScaleType(ScaleType.CENTER_INSIDE);
				((TextView) childview.findViewById(R.id.game_text)).setVisibility(View.GONE);
			}
			return bitmap;
		} else {
			new GameImage(childview, boxart).execute(key);
			return null;
		}
	}
	
	public class GameImage extends AsyncTask<String, Integer, Bitmap> {
		
		private View childview;
		private String key;
		private ImageView preview;
		private String boxart;
		
		public GameImage(View childview, String boxart) {
			this.childview = childview;
			this.boxart = boxart;
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
				} else {
					api = "http://thegamesdb.net/banners/" + boxart;
				}
				try {
					URL imageURL = new URL(api);
					URLConnection conn1 = imageURL.openConnection();
					
					InputStream im = conn1.getInputStream();
					BufferedInputStream bis = new BufferedInputStream(im, 512);
					
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
					
					bis.close();
					im.close();
					bis = null;
					im = null;
					return bitmap;
				} catch (IOException e) {
					
				}
			}
			return null;
		}
		
		@Override
		protected void onPostExecute(Bitmap image) {
			if (image != null) {
				saveImage(key, image);
				if (preview != null) {
					preview.setImageBitmap(image);
					preview.setScaleType(ScaleType.CENTER_INSIDE);
					((TextView) childview.findViewById(R.id.game_text)).setVisibility(View.GONE);
				}
			}
		}
	}
	
	public OnLongClickListener configureLongClick(final String title, final String overview, final File gameFile) {
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
						MainActivity.launchGame(gameFile);
						return;
					}
				});
				builder.create().show();
				return true;
			}
		};
	}
	
	public String[] getGameInfo(File game, View childview) {
		String serial = getSerial(game);
		if (serial == null) {
			getImage(game.getName(), childview, null);
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
			return new String[] { gameID, title, overview, boxart };
		} else {
			GamesDbAPI gameDatabase = new GamesDbAPI(mContext, gameID);
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
