package com.virtualapplications.play;

import java.io.BufferedInputStream;
import java.io.File;
import java.io.IOException;
import java.io.InputStream;
import java.io.StringReader;
import java.io.UnsupportedEncodingException;
import java.net.HttpURLConnection;
import java.net.URL;
import java.net.URLConnection;

import org.apache.http.HttpEntity;
import org.apache.http.HttpResponse;
import org.apache.http.client.ClientProtocolException;
import org.apache.http.client.methods.HttpPost;
import org.apache.http.impl.client.DefaultHttpClient;

import android.content.Context;
import android.content.ContextWrapper;
import android.content.ContentResolver;
import android.database.Cursor;
import android.database.SQLException;
import android.graphics.Bitmap;
import android.graphics.BitmapFactory;
import android.os.AsyncTask;
import android.os.Build;
import android.os.StrictMode;
import android.util.SparseArray;
import android.view.View;
import android.widget.ImageView;
import android.widget.ImageView.ScaleType;
import android.widget.TextView;

import java.io.FileNotFoundException;
import java.io.FileOutputStream;
import java.io.OutputStream;

import java.util.concurrent.ExecutionException;

public class GameInfo {
	
	private Context mContext;
	
	public GameInfo (Context mContext) {
		this.mContext = mContext;
	}
	
	public Bitmap decodeBitmapIcon(String filename) throws IOException {
		URL updateURL = new URL(filename);
		URLConnection conn1 = updateURL.openConnection();
		InputStream im = conn1.getInputStream();
		BufferedInputStream bis = new BufferedInputStream(im, 512);
		
		BitmapFactory.Options options = new BitmapFactory.Options();
		options.inJustDecodeBounds = true;
		Bitmap bitmap = BitmapFactory.decodeStream(bis, null, options);
		
		options.inSampleSize = calculateInSampleSize(options);
		options.inJustDecodeBounds = false;
		bis.close();
		im.close();
		conn1 = updateURL.openConnection();
		im = conn1.getInputStream();
		bis = new BufferedInputStream(im, 512);
		bitmap = BitmapFactory.decodeStream(bis, null, options);
		
		bis.close();
		im.close();
		bis = null;
		im = null;
		return bitmap;
	}
	
	private int calculateInSampleSize(BitmapFactory.Options options) {
		final int height = options.outHeight;
		final int width = options.outWidth;
		final int reqHeight = 400;
		final int reqWidth = 300;
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
	
	public Bitmap getImage(String key, View childview) {
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
			new GameImage(childview).execute(key);
			return null;
		}
	}
	
	public class GameImage extends AsyncTask<String, Integer, Bitmap> {
		
		private View childview;
		private String key;
		
		public GameImage(View childview) {
			this.childview = childview;
		}
		
		protected void onPreExecute() {
			
		}
		
		@Override
		protected Bitmap doInBackground(String... params) {
			key = params[0];
			if (GamesDbAPI.isNetworkAvailable(mContext)) {
				String api = "http://thegamesdb.net/banners/boxart/thumb/original/front/" + key + "-1.jpg";
				try {
					URL url = new URL(api);
					HttpURLConnection connection = (HttpURLConnection) url.openConnection();
					connection.setDoInput(true);
					connection.connect();
					InputStream input = connection.getInputStream();
					return BitmapFactory.decodeStream(input);
				} catch (IOException e) {
					
				}
			}
			return null;
		}
		
		@Override
		protected void onPostExecute(Bitmap image) {
			if (image != null) {
				saveImage(key, image);
				if (childview != null) {
					ImageView preview = (ImageView) childview.findViewById(R.id.game_icon);
					preview.setImageBitmap(image);
					preview.setScaleType(ScaleType.CENTER_INSIDE);
					((TextView) childview.findViewById(R.id.game_text)).setVisibility(View.GONE);
				}
			}
		}
	}
	
	public void getDatabase() {
		ContextWrapper cw = new ContextWrapper(mContext);
		String DB_PATH = cw.getFilesDir().getAbsolutePath()+ "/databases/";
		String DB_NAME = "games.db";
		
		if (!new File (DB_PATH + DB_NAME).exists()) {
			byte[] buffer = new byte[1024];
			OutputStream myOutput = null;
			int length;
			InputStream myInput = null;
			try
			{
				myInput = mContext.getAssets().open(DB_NAME);
				myOutput = new FileOutputStream(DB_PATH + DB_NAME);
				while((length = myInput.read(buffer)) > 0)
				{
					myOutput.write(buffer, 0, length);
				}
				myOutput.close();
				myOutput.flush();
				myInput.close();
				
			}
			catch(IOException e)
			{
				e.printStackTrace();
			}
		}
	}
	
	public String[] getGameInfo(File game, View childview) {
		String serial = getSerial(game);
		String gameID = null;
		ContentResolver cr = mContext.getContentResolver();
		String selection = TheGamesDB.KEY_SERIAL + "=?";
		String[] selectionArgs = { serial };
		Cursor c = cr.query(TheGamesDB.GAMESDB_URI, null, selection, selectionArgs, TheGamesDB.KEY_SERIAL + " desc");
		if (c.moveToFirst()) {
			gameID = c.getString(c.getColumnIndex(TheGamesDB.KEY_GAMEID));
			String title = c.getString(c.getColumnIndex(TheGamesDB.KEY_TITLE));
			String overview = c.getString(c.getColumnIndex(TheGamesDB.KEY_OVERVIEW));
			if (!overview.equals(null)) {
				return new String[] { gameID, title, overview };
			}
		}
		if (gameID != null) {
			GamesDbAPI gameDatabase = new GamesDbAPI(mContext, gameID);
			gameDatabase.setView(childview);
			gameDatabase.execute(game);
		}
		return null;
	}
	
	String[] SerialDummy = new String[]{"SLPM_670.10", "SLPM_661.51", "SLUS_210.05", "SLPS_250.50", "SLUS_209.63", "SCUS_974.72"};
	public static int tmpint = -1;
	
	public String getSerial(File game) {
		//TODO:A Call To Native Call
		if (tmpint >= 5) tmpint=-1;
		return SerialDummy[++tmpint];
	}
}
