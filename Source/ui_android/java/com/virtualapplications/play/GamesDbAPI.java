package com.virtualapplications.play;

import java.io.BufferedInputStream;
import java.io.IOException;
import java.io.InputStream;
import java.io.StringReader;
import java.io.UnsupportedEncodingException;
import java.net.URL;
import java.net.URLConnection;

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
import android.graphics.Bitmap;
import android.graphics.BitmapFactory;
import android.graphics.drawable.BitmapDrawable;
import android.net.ConnectivityManager;
import android.net.NetworkInfo;
import android.widget.ImageView;

public class GamesDbAPI{

	private ImageView preview;

	private static final String games_url_name = "http://thegamesdb.net/api/GetGame.php?platform=sony+playstation+2&name=";
	private static final String games_url_id = "http://thegamesdb.net/api/GetGame.php?platform=sony+playstation+2&id=";
	private static final String gameslist_url = "http://thegamesdb.net/api/PlatformGames.php?platform=sony+playstation+2";

	public GamesDbAPI() {
	}

	public GameInfo getCover(GameInfo TheGame, Context mContext, ImageView preview){
		this.preview= preview;
		String filename = TheGame.getName();
		int gameID = TheGame.getID();

		if (isNetworkAvailable(mContext)) {
			try {
				DefaultHttpClient httpClient = new DefaultHttpClient();
				HttpPost httpPost;
				if (gameID != 0){
					httpPost = new HttpPost(games_url_id + gameID);
				} else {
					if (filename.startsWith("[")) {
						filename = filename.substring(filename.indexOf("]") + 1, filename.length());
					}
					if (filename.contains("[")) {
						filename = filename.substring(0, filename.indexOf("["));
					} else {
						if (filename.contains(".") && (filename.length() - 4 - filename.lastIndexOf(".")) == 0)
							filename = filename.substring(0, filename.lastIndexOf("."));
					}
					filename = filename.replace("_", " ").replace(":", " ");
					filename = filename.replaceAll("[^\\p{Alpha}\\p{Digit}]+"," ");
					filename = filename.replace("  ", " ").replace(" ", "+");
					if (filename.endsWith("+")) {
						filename = filename.substring(0, filename.length() - 1);
					}
					httpPost = new HttpPost(games_url_name + filename);
				}
				HttpResponse httpResponse = httpClient.execute(httpPost);
				HttpEntity httpEntity = httpResponse.getEntity();
				String gameData =  EntityUtils.toString(httpEntity);
				if (gameData != null) {
					Document doc = getDomElement(gameData);
					if (doc.getElementsByTagName("Game") != null && doc.getElementsByTagName("Game").getLength() > 0) {
						Element root = (Element) doc.getElementsByTagName("Game").item(0);
						String details = getValue(root, "Overview");
						TheGame.setDescription(details);
						if (TheGame.getID() != 0){
							TheGameDB db = new TheGameDB(mContext);
							db.updateGame(TheGame, details);
						}
						Element images = (Element) root.getElementsByTagName("Images").item(0);
						Element sleave = (Element) images.getElementsByTagName("boxart").item(0);
						//TODO: to keep or not to keep
						String cover = "http://thegamesdb.net/banners/" + getElementValue(sleave);
						BitmapDrawable game_icon = new BitmapDrawable(decodeBitmapIcon(cover));
						Element boxart = (Element) images.getElementsByTagName("boxart").item(1);
						String image = "http://thegamesdb.net/banners/" + getElementValue(boxart);
						Bitmap game_image = decodeBitmapIcon(image);
						TheGame.setFrontCover(game_image);
						TheGame.setBackCover(game_icon.getBitmap());
					}
				}
			} catch (UnsupportedEncodingException e) {

			} catch (ClientProtocolException e) {

			} catch (IOException e) {

			}
		}

		return TheGame;
	}

	public static String getDescription(String filename, int gameID, Context mContext){

		if (isNetworkAvailable(mContext)) {
			try {
				DefaultHttpClient httpClient = new DefaultHttpClient();
				HttpPost httpPost;
				if (gameID != 0){
					httpPost = new HttpPost(games_url_id + gameID);
				} else {
					if (filename.startsWith("[")) {
						filename = filename.substring(filename.indexOf("]") + 1, filename.length());
					}
					if (filename.contains("[")) {
						filename = filename.substring(0, filename.indexOf("["));
					} else {
						if (filename.contains(".") && (filename.lastIndexOf(".") - filename.length() - 4) == 0)
						filename = filename.substring(0, filename.lastIndexOf("."));
					}
					filename = filename.replace("_", " ").replace(":", " ");
					filename = filename.replaceAll("[^\\p{Alpha}\\p{Digit}]+"," ");
					filename = filename.replace("  ", " ").replace(" ", "+");
					if (filename.endsWith("+")) {
						filename = filename.substring(0, filename.length() - 1);
					}
					httpPost = new HttpPost(games_url_name + filename);
				}
				HttpResponse httpResponse = httpClient.execute(httpPost);
				HttpEntity httpEntity = httpResponse.getEntity();
				String gameData =  EntityUtils.toString(httpEntity);
				if (gameData != null) {
					Document doc = getDomElement(gameData);
					if (doc.getElementsByTagName("Game") != null) {
						Element root = (Element) doc.getElementsByTagName("Game").item(0);
						String details = getValue(root, "Overview");

						return details;
					}
				}
			} catch (UnsupportedEncodingException e) {

			} catch (ClientProtocolException e) {

			} catch (IOException e) {

			}
		}
		return null;
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


	public static boolean isNetworkAvailable(Context mContext) {
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


	public static Document getDomElement(String xml) {
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

	public static String getValue(Element item, String str) {
		NodeList n = item.getElementsByTagName(str);
		return getElementValue(n.item(0));
	}

	public static final String getElementValue(Node elem) {
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

	public static String getGameDBList(Context mContext){
		if (isNetworkAvailable(mContext)) {
			try {
				DefaultHttpClient httpClient = new DefaultHttpClient();
				HttpPost httpPost;
				httpPost = new HttpPost(gameslist_url);
				HttpResponse httpResponse = httpClient.execute(httpPost);
				HttpEntity httpEntity = httpResponse.getEntity();
				String gameData =  EntityUtils.toString(httpEntity);
				if (gameData != null) {
					Document doc = getDomElement(gameData);
					if (doc.getElementsByTagName("Game") != null) {
						NodeList root = doc.getElementsByTagName("Game");
						TheGameDB db = new TheGameDB(mContext);
						for(int i = 0; i < root.getLength(); i++) {
							String ID = getValue((Element) root.item(i), "id");
							String gameName = getValue((Element) root.item(i), "GameTitle");
							String cover = getValue((Element) root.item(i), "thumb");
							db.addGame(new GameInfo(Integer.parseInt(ID),gameName, null, cover));
						}
					}
				}

			} catch (UnsupportedEncodingException e) {

			} catch (ClientProtocolException e) {

			} catch (IOException e) {

			}
		}
		return null;
	}
}
