package com.virtualapplications.play;

import java.io.BufferedInputStream;
import java.io.File;
import java.io.IOException;
import java.io.InputStream;
import java.io.StringReader;
import java.io.UnsupportedEncodingException;
import java.net.URL;
import java.net.URLConnection;
import java.net.URLEncoder;

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
import android.content.ContentValues;
import android.graphics.Bitmap;
import android.graphics.BitmapFactory;
import android.graphics.drawable.BitmapDrawable;
import android.graphics.drawable.Drawable;
import android.net.ConnectivityManager;
import android.net.NetworkInfo;
import android.os.AsyncTask;
import android.os.Build;
import android.os.StrictMode;
import android.view.View;
import android.view.View.OnLongClickListener;
import android.widget.ImageView;
import android.widget.ImageView.ScaleType;
import android.widget.TextView;

import com.virtualapplications.play.SqliteHelper.Games;

public class GamesDbAPI extends AsyncTask<File, Integer, Document> {

	private int index;
	private View childview;
	private Context mContext;
	private String gameID;
	private File gameFile;
	private GameInfo gameInfo;
	private boolean elastic;

	private static final String games_url = "http://thegamesdb.net/api/GetGamesList.php?platform=sony+playstation+2&name=";
    private static final String games_url_id = "http://thegamesdb.net/api/GetGame.php?platform=sony+playstation+2&id=";
	private static final String games_list = "http://thegamesdb.net/api/GetPlatformGames.php?platform=11";

	public GamesDbAPI(Context mContext, String gameID) {
		this.elastic = false;
		this.mContext = mContext;
		this.gameID = gameID;
	}
	
	public void setView(View childview) {
		this.childview = childview;
	}

	protected void onPreExecute() {
		gameInfo = new GameInfo(mContext);
		if (Build.VERSION.SDK_INT >= Build.VERSION_CODES.GINGERBREAD) {
			StrictMode.ThreadPolicy policy = new StrictMode.ThreadPolicy.Builder()
					.permitAll().build();
			StrictMode.setThreadPolicy(policy);
		}
	}

	@Override
	protected Document doInBackground(File... params) {
		
		if (GamesDbAPI.isNetworkAvailable(mContext)) {
			try {
				DefaultHttpClient httpClient = new DefaultHttpClient();
				HttpPost httpPost;
				if (params[0] != null) {
					gameFile = params[0];
					String filename = gameFile.getName();
					if (gameID != null) {
						httpPost = new HttpPost(games_url_id + gameID);
					} else {
						elastic = true;
						filename = filename.substring(0, filename.lastIndexOf("."));
						try {
							filename = URLEncoder.encode(filename, "UTF-8");
						} catch (UnsupportedEncodingException e) {
							filename = filename.replace(" ", "+");
						}
						httpPost = new HttpPost(games_url + filename);
					}
				} else {
					httpPost = new HttpPost(games_list);
				}
				HttpResponse httpResponse = httpClient.execute(httpPost);
				HttpEntity httpEntity = httpResponse.getEntity();
				String gameData =  EntityUtils.toString(httpEntity);
				if (gameData != null) {
					return getDomElement(gameData);
				} else {
					return null;
				}
			} catch (UnsupportedEncodingException e) {

			} catch (ClientProtocolException e) {

			} catch (IOException e) {

			}
		}
		return null;
	}

	@Override
	protected void onPostExecute(Document doc) {
		
		if (doc.getElementsByTagName("Game") != null) {
			try {
				final Element root = (Element) doc.getElementsByTagName("Game").item(0);
				
				final String remoteID = getValue(root, "id");
				
				if (elastic) {
					GamesDbAPI gameDatabase = new GamesDbAPI(mContext, remoteID);
					gameDatabase.setView(childview);
					gameDatabase.execute(gameFile);
				} else {
					ContentValues values = new ContentValues();
					final String overview = getValue(root, "Overview");
					values.put(Games.KEY_OVERVIEW, overview);
					
					Element images = (Element) root.getElementsByTagName("Images").item(0);
					Element boxart = null;
					if (images.getElementsByTagName("boxart").getLength() > 1) {
						boxart = (Element) images.getElementsByTagName("boxart").item(1);
					} else if (images.getElementsByTagName("boxart").getLength() == 1) {
						boxart = (Element) images.getElementsByTagName("boxart").item(0);
					}
					String coverImage = null;
					if (boxart != null) {
						coverImage = getElementValue(boxart);
						values.put(Games.KEY_BOXART, coverImage);
					} else {
						values.put(Games.KEY_BOXART, "404");
					}
					
					if (gameID != null) {

						String selection = Games.KEY_GAMEID + "=?";
						String[] selectionArgs = { gameID };
						
						mContext.getContentResolver().update(Games.GAMES_URI, values, selection, selectionArgs);
						
					} else {
						
						String selection = Games.KEY_GAMEID + "=?";
						String[] selectionArgs = { remoteID };
						
						mContext.getContentResolver().update(Games.GAMES_URI, values, selection, selectionArgs);
						
					}
					if (childview != null) {
						childview.findViewById(R.id.childview).setOnLongClickListener(
							gameInfo.configureLongClick(getValue(root, "GameTitle"), overview, gameFile));
						if (coverImage != null) {
							if (gameID != null) {
								gameInfo.getImage(gameID, childview, coverImage);
							} else {
								gameInfo.getImage(remoteID, childview, coverImage);
							}
						}
					}
				}
			} catch (Exception e) {
				
			}
		}
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

}
