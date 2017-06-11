package com.virtualapplications.play.database;

import java.io.File;
import java.io.InputStream;
import java.io.IOException;
import java.io.StringReader;
import java.io.UnsupportedEncodingException;
import java.net.URL;
import java.net.URLEncoder;
import java.net.HttpURLConnection;

import javax.xml.parsers.DocumentBuilder;
import javax.xml.parsers.DocumentBuilderFactory;
import javax.xml.parsers.ParserConfigurationException;

import org.apache.commons.io.IOUtils;
import org.w3c.dom.Document;
import org.w3c.dom.Element;
import org.w3c.dom.Node;
import org.w3c.dom.NodeList;
import org.xml.sax.InputSource;
import org.xml.sax.SAXException;

import android.content.Context;
import android.net.ConnectivityManager;
import android.net.NetworkInfo;
import android.os.AsyncTask;
import android.os.Build;
import android.os.StrictMode;
import android.util.Log;

import com.virtualapplications.play.Constants;
import com.virtualapplications.play.GameInfoStruct;
import com.virtualapplications.play.GamesAdapter;

public class GamesDbAPI extends AsyncTask<File, Integer, Boolean> {

	private final GameInfoStruct gameInfoStruct;
	private final int pos;
	private GamesAdapter.CoverViewHolder viewHolder;
	private Context mContext;
	private String gameID;
	private GameInfo gameInfo;
	private boolean elastic;

	private static final String games_url = "http://thegamesdb.net/api/GetGamesList.php?platform=sony+playstation+2&name=";
    private static final String games_url_id = "http://thegamesdb.net/api/GetGame.php?platform=sony+playstation+2&id=";
	private static final String games_list = "http://thegamesdb.net/api/GetPlatformGames.php?platform=11";

	public GamesDbAPI(Context mContext, GameInfoStruct gameInfoStruct, int pos) {
		this.elastic = false;
		this.mContext = mContext;
		this.gameID = gameInfoStruct.getGameID();
		this.pos = pos;
		this.gameInfoStruct = gameInfoStruct;
	}
	
	public void setView(GamesAdapter.CoverViewHolder viewHolder) {
		this.viewHolder = viewHolder;
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
	protected Boolean doInBackground(File... params) {
		
		if (GamesDbAPI.isNetworkAvailable(mContext)) {
			try {
				URL requestUrl;
				if (gameInfoStruct.getFile() != null) {
					String filename = gameInfoStruct.getFile().getName();
					if (gameID != null) {
						requestUrl = new URL(games_url_id + gameID);
					} else {
						elastic = true;
						filename = filename.substring(0, filename.lastIndexOf("."));
						try {
							filename = URLEncoder.encode(filename, "UTF-8");
						} catch (UnsupportedEncodingException e) {
							filename = filename.replace(" ", "+");
						}
						requestUrl = new URL(games_url + filename);
					}
				} else {
					requestUrl = new URL(games_list);
				}
				HttpURLConnection urlConnection = (HttpURLConnection)requestUrl.openConnection();
				urlConnection.setRequestMethod("POST");
				try 
				{
					InputStream inputStream = urlConnection.getInputStream();
					String gameData = IOUtils.toString(inputStream, "UTF-8");
					Document doc = getDomElement(gameData);
					if (doc != null && doc.getElementsByTagName("Game") != null) {
						try {
							final Element root = (Element) doc.getElementsByTagName("Game").item(0);
							final String remoteID = getValue(root, "id");

							if (elastic) {
								this.gameID = remoteID;
								this.elastic = false;
								return doInBackground();
							} else {
								final String title = getValue(root, "GameTitle");
								final String overview = getValue(root, "Overview");

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
								}

								if (gameInfoStruct.getGameID() == null){
									gameInfoStruct.setGameID(remoteID, mContext);
								}
								if (gameInfoStruct.isTitleNameEmptyNull()){
									gameInfoStruct.setTitleName(title, mContext);
								}
								if (gameInfoStruct.isDescriptionEmptyNull()){
									gameInfoStruct.setDescription(overview, mContext);
								}
								if (gameInfoStruct.getFrontLink() == null || gameInfoStruct.getFrontLink().isEmpty()){
									if (coverImage != null) {
										gameInfoStruct.setFrontLink(coverImage, mContext);
									}
								}

								return true;
							}
						} catch (Exception e) {

						}
					}
				}
				catch(Exception ex)
				{
					Log.w(Constants.TAG, String.format("Failed to obtain information: %s", ex.toString()));
				}
				finally 
				{
					urlConnection.disconnect();
				}
				
			} catch (UnsupportedEncodingException e) {

			} catch (IOException e) {

			}
		}
		return false;
	}

	@Override
	protected void onPostExecute(Boolean status) {
		if (status)
		{
			if (viewHolder != null) {
				viewHolder.childview.setOnLongClickListener(
						gameInfo.configureLongClick(gameInfoStruct));
				if (gameInfoStruct.getFrontLink() != null) {
					gameInfo.setCoverImage(gameInfoStruct.getGameID(), viewHolder, gameInfoStruct.getFrontLink(), pos);
				}
			}
		}
		else
		{
			if (gameInfoStruct.isTitleNameEmptyNull()){
				gameInfoStruct.setTitleName(gameInfoStruct.getFile().getName(), mContext);
			}
			if (gameInfoStruct.isDescriptionEmptyNull()){
				gameInfoStruct.setDescription("No Game Description Found", mContext);
			}
			if (gameInfoStruct.getFrontLink() == null || gameInfoStruct.getFrontLink().isEmpty()){
					gameInfoStruct.setFrontLink("404", mContext);
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
