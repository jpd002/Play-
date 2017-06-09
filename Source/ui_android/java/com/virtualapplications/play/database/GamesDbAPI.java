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
import android.content.ContentValues;
import android.content.ContentResolver;
import android.database.Cursor;
import android.net.ConnectivityManager;
import android.net.NetworkInfo;
import android.os.AsyncTask;
import android.os.Build;
import android.os.StrictMode;
import android.util.Log;
import android.view.View;

import com.virtualapplications.play.Constants;
import com.virtualapplications.play.GameInfoStruct;
import com.virtualapplications.play.GamesAdapter;
import com.virtualapplications.play.R;
import com.virtualapplications.play.database.SqliteHelper.Games;

public class GamesDbAPI extends AsyncTask<File, Integer, Boolean> {

	private final String serial;
	private final GameInfoStruct gameInfoStruct;
	private final int pos;
	private int index;
	private GamesAdapter.CoverViewHolder viewHolder;
	private Context mContext;
	private String gameID;
	private File gameFile;
	private GameInfo gameInfo;
	private boolean elastic;

	private static final String games_url = "http://thegamesdb.net/api/GetGamesList.php?platform=sony+playstation+2&name=";
    private static final String games_url_id = "http://thegamesdb.net/api/GetGame.php?platform=sony+playstation+2&id=";
	private static final String games_list = "http://thegamesdb.net/api/GetPlatformGames.php?platform=11";
	private boolean _terminate = true;

	public GamesDbAPI(Context mContext, String gameID, String serial, GameInfoStruct gameInfoStruct, int pos) {
		this.elastic = false;
		this.mContext = mContext;
		this.gameID = gameID;
		this.serial = serial;
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
				if (params[0] != null) {
					gameFile = params[0];
					String filename = gameFile.getName();
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

							ContentResolver cr = mContext.getContentResolver();
							String selection = Games.KEY_GAMEID + "=?";
							String[] selectionArgs = { remoteID };
							Cursor c = cr.query(Games.GAMES_URI, null, selection, selectionArgs, null);
							String dataID = null;

							if (elastic) {
								if (c != null && c.getCount() > 0) {
									if (c.moveToFirst()) {
										do {
											String title = c.getString(c.getColumnIndex(Games.KEY_TITLE));
											String overview = c.getString(c.getColumnIndex(Games.KEY_OVERVIEW));
											String boxart = c.getString(c.getColumnIndex(Games.KEY_BOXART));
											if (overview != null && boxart != null &&
													!overview.equals("") && !boxart.equals("")) {
												dataID = c.getString(c.getColumnIndex(Games.KEY_GAMEID));
												if (gameInfoStruct.getGameID() == null){
													gameInfoStruct.setGameID(dataID, null);
												}
												if (gameInfoStruct.isTitleNameEmptyNull()){
													gameInfoStruct.setTitleName(title, null);
												}
												if (gameInfoStruct.isDescriptionEmptyNull()){
													gameInfoStruct.setDescription(overview, null);
												}
												if (gameInfoStruct.getFrontLink() == null || gameInfoStruct.getFrontLink().isEmpty()){
													gameInfoStruct.setFrontLink(boxart, null);
												}
												c.close();
												return true;
											}
										} while (c.moveToNext());
									}
								}
								c.close();
								if (dataID == null) {
									this.gameID = remoteID;
									_terminate = false;
									return false;
								}
							} else {
								ContentValues values = new ContentValues();
								values.put(Games.KEY_GAMEID, remoteID);
								final String title = getValue(root, "GameTitle");
								values.put(Games.KEY_TITLE, title);
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

								/*
								TODO:Adding the serial to the database this way is not ideal, as a mismatch will persist
								but as it stands that's the only way to get the pictures and prevent them from being downloaded every run
								 */
								if (c != null && c.getCount() > 0) {
									if (c.moveToFirst()) {
										do {
											String db_serial = c.getString(c.getColumnIndex(Games.KEY_SERIAL));
											if (db_serial == null || db_serial == serial){
												values.put(Games.KEY_SERIAL, serial);
												mContext.getContentResolver().update(Games.GAMES_URI, values, selection, selectionArgs);
												break;
											} else {
												values.remove(Games.KEY_SERIAL);
												mContext.getContentResolver().update(Games.GAMES_URI, values, selection, selectionArgs);
											}


										} while (c.moveToNext());

									}
								} else {
									values.put(Games.KEY_SERIAL, serial);
									mContext.getContentResolver().insert(Games.GAMES_URI, values);
								}
								c.close();

								String m_title = getValue(root, "GameTitle");

								if (gameInfoStruct.getGameID() == null){
									gameInfoStruct.setGameID(remoteID, null);
								}
								if (gameInfoStruct.isTitleNameEmptyNull()){
									gameInfoStruct.setTitleName(m_title, null);
								}
								if (gameInfoStruct.isDescriptionEmptyNull()){
									gameInfoStruct.setDescription(overview, null);
								}
								if (gameInfoStruct.getFrontLink() == null || gameInfoStruct.getFrontLink().isEmpty()){
									gameInfoStruct.setFrontLink(coverImage, null);
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
		if (status) {
			if (viewHolder != null) {
				viewHolder.childview.setOnLongClickListener(
						gameInfo.configureLongClick(gameInfoStruct));
				if (gameInfoStruct.getFrontLink() != null) {
					gameInfo.setCoverImage(gameInfoStruct.getGameID(), viewHolder, gameInfoStruct.getFrontLink(), pos);
				}
			}
		}
		else if(!_terminate)
		{
			GamesDbAPI gameDatabase = new GamesDbAPI(mContext, gameID, serial, this.gameInfoStruct, pos);
			gameDatabase.setView(viewHolder);
			gameDatabase.execute(gameFile);
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
