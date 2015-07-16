package com.virtualapplications.play;

import java.io.BufferedInputStream;
import java.io.File;
import java.io.IOException;
import java.io.InputStream;
import java.io.StringReader;
import java.io.UnsupportedEncodingException;
import java.net.URL;
import java.net.URLConnection;
import java.util.Locale;

import javax.xml.parsers.DocumentBuilder;
import javax.xml.parsers.DocumentBuilderFactory;
import javax.xml.parsers.ParserConfigurationException;

import org.apache.http.HttpEntity;
import org.apache.http.HttpResponse;
import org.apache.http.client.ClientProtocolException;
import org.apache.http.client.methods.HttpPost;
import org.apache.http.impl.client.DefaultHttpClient;
import org.apache.http.util.EntityUtils;
import org.w3c.dom.Document;
import org.w3c.dom.Element;
import org.w3c.dom.Node;
import org.w3c.dom.NodeList;
import org.xml.sax.InputSource;
import org.xml.sax.SAXException;

import android.content.Context;
import android.content.SharedPreferences;
import android.graphics.Bitmap;
import android.graphics.BitmapFactory;
import android.graphics.drawable.BitmapDrawable;
import android.graphics.drawable.Drawable;
import android.net.ConnectivityManager;
import android.net.NetworkInfo;
import android.os.AsyncTask;
import android.os.Build;
import android.os.StrictMode;
import android.util.SparseArray;
import android.view.View;
import android.widget.ImageView;
import android.widget.ImageView.ScaleType;

public class GamesDbAPI extends AsyncTask<String, Integer, String> {

	private File game;
	private int index;
	private View childview;
	private ImageView preview;
	private Context mContext;
	private String game_name;
	private Drawable game_icon;

	private static final String games_url = "http://thegamesdb.net/api/GetGame.php?platform=sony+playstation+2&name=";
	public SparseArray<String> game_details = new SparseArray<String>();
	public SparseArray<Bitmap> game_preview = new SparseArray<Bitmap>();

	public GamesDbAPI(File game, int index) {
		this.game = game;
		this.index = index;
	}

	public void setViewParent(Context mContext, View childview) {
		this.mContext = mContext;
		this.childview = childview;
	}

	protected void onPreExecute() {
		preview = (ImageView) childview.findViewById(R.id.game_icon);
		if (Build.VERSION.SDK_INT >= Build.VERSION_CODES.GINGERBREAD) {
			StrictMode.ThreadPolicy policy = new StrictMode.ThreadPolicy.Builder()
					.permitAll().build();
			StrictMode.setThreadPolicy(policy);
		}
	}

	public Bitmap decodeBitmapIcon(String filename) throws IOException {
		URL updateURL = new URL(filename);
		URLConnection conn1 = updateURL.openConnection();
		InputStream im = conn1.getInputStream();
		BufferedInputStream bis = new BufferedInputStream(im, 512);

		BitmapFactory.Options options = new BitmapFactory.Options();
		options.inJustDecodeBounds = true;
		Bitmap bitmap = BitmapFactory.decodeStream(bis, null, options);

		options.inSampleSize = calculateInSampleSize(options, preview);
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
	
	private int calculateInSampleSize(BitmapFactory.Options options, ImageView imageView) {
		final int height = options.outHeight;
		final int width = options.outWidth;
		final int reqHeight = imageView.getMeasuredHeight();
		final int reqWidth = imageView.getMeasuredWidth();
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
	protected String doInBackground(String... params) {
		String filename = game_name = params[0];
		if (isNetworkAvailable()) {
			if (filename.startsWith("[")) {
				filename = filename.substring(filename.indexOf("]") + 1, filename.length());
			}
			if (filename.contains("[")) {
				filename = filename.substring(0, filename.indexOf("["));
			} else {
				filename = filename.substring(0, filename.lastIndexOf("."));
			}
			filename = filename.replace("_", " ").replace(":", " ");
			filename = filename.replaceAll("[^\\p{Alpha}\\p{Digit}]+"," ");
			filename = filename.replace("  ", " ").replace(" ", "+");
			if (filename.endsWith("+")) {
				filename = filename.substring(0, filename.length() - 1);
			}
			try {
				DefaultHttpClient httpClient = new DefaultHttpClient();
				HttpPost httpPost = new HttpPost(games_url + filename);

				HttpResponse httpResponse = httpClient.execute(httpPost);
				HttpEntity httpEntity = httpResponse.getEntity();
				return EntityUtils.toString(httpEntity);
			} catch (UnsupportedEncodingException e) {

			} catch (ClientProtocolException e) {

			} catch (IOException e) {

			}
		}
		return null;
	}

	@Override
	protected void onPostExecute(String gameData) {
		if (gameData != null) {
			Document doc = getDomElement(gameData);
			if (doc != null && doc.getElementsByTagName("Game") != null) {
				Element root = (Element) doc.getElementsByTagName("Game").item(
						0);
				game_name = getValue(root, "GameTitle");
				String details = getValue(root, "Overview");
				game_details.put(index, details);
				Element images = (Element) root.getElementsByTagName("Images").item(0);
				Element boxart = (Element) images.getElementsByTagName("boxart").item(1);
				String image = "http://thegamesdb.net/banners/" + getElementValue(boxart);
				try {
					Bitmap game_image = decodeBitmapIcon(image);
					game_preview.put(index, game_image);
					preview.setImageBitmap(game_image);
					preview.setScaleType(ScaleType.CENTER_INSIDE);
				} catch (IOException e) {
					// TODO Auto-generated catch block
					e.printStackTrace();
				}
			}
		} else {
			game_details.put(index,
					mContext.getString(R.string.game_info));
			final String nameLower = game.getName().toLowerCase(
					Locale.getDefault());
		}

		childview.setTag(game_name);
	}

	public boolean isNetworkAvailable() {
		ConnectivityManager connectivityManager = (ConnectivityManager) mContext
				.getSystemService(Context.CONNECTIVITY_SERVICE);		
		NetworkInfo mWifi = connectivityManager.getNetworkInfo(ConnectivityManager.TYPE_WIFI);
		NetworkInfo mMobile = connectivityManager.getNetworkInfo(ConnectivityManager.TYPE_MOBILE);
		NetworkInfo activeNetworkInfo = connectivityManager.getActiveNetworkInfo();
		if (mMobile != null && mWifi != null) {
			return mMobile.isAvailable() || mWifi.isAvailable();
		} else {
			return activeNetworkInfo != null && activeNetworkInfo.isConnected();
		}
	}

	public Drawable getGameIcon() {
		return game_icon;
	}

	public String getGameTitle() {
		return game_name;
	}

	public Document getDomElement(String xml) {
		Document doc = null;
		DocumentBuilderFactory dbf = DocumentBuilderFactory.newInstance();
		try {

			DocumentBuilder db = dbf.newDocumentBuilder();

			InputSource is = new InputSource();
			is.setCharacterStream(new StringReader(xml));
			doc = db.parse(is);

		} catch (ParserConfigurationException e) {

			return null;
		} catch (SAXException e) {

			return null;
		} catch (IOException e) {

			return null;
		}

		return doc;
	}

	public String getValue(Element item, String str) {
		NodeList n = item.getElementsByTagName(str);
		return this.getElementValue(n.item(0));
	}

	public final String getElementValue(Node elem) {
		Node child;
		if (elem != null) {
			if (elem.hasChildNodes()) {
				for (child = elem.getFirstChild(); child != null; child = child
						.getNextSibling()) {
					if (child.getNodeType() == Node.TEXT_NODE) {
						return child.getNodeValue();
					}
				}
			}
		}
		return "";
	}
}
