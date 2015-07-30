package com.virtualapplications.play;

import android.content.ContentProvider;
import android.content.ContentUris;
import android.content.ContentValues;
import android.content.Context;
import android.content.UriMatcher;
import android.database.Cursor;
import android.database.SQLException;
import android.database.sqlite.SQLiteDatabase;
import android.database.sqlite.SQLiteOpenHelper;
import android.database.sqlite.SQLiteQueryBuilder;
import android.net.Uri;
import android.text.TextUtils;
import android.util.Log;

import java.io.FileNotFoundException;
import java.io.FileOutputStream;
import java.io.InputStream;
import java.io.IOException;
import java.io.OutputStream;

import java.util.ArrayList;
import java.util.HashMap;
import java.util.List;


import com.virtualapplications.play.SqliteHelper.Games;
import com.virtualapplications.play.SqliteHelper.Covers;

public class TheGamesDB extends ContentProvider {
	
	// Database Name
	private static final String DATABASE_NAME = "games.db";

    // Database Version
    private static final int DATABASE_VERSION = 1;
	
	public static final String SQLITE_ID = "_id";

	private static final UriMatcher sUriMatcher;
	
	private static final int GAMES = 1;
	private static final int GAMESID = 2;
	private static final int COVERS = 3;
	private static final int COVERSID = 4;
	
	private static HashMap<String, String> gamesMap;
	private static HashMap<String, String> coversMap;
	
	private static class DatabaseHelper extends SQLiteOpenHelper {
		
		private Context mContext;

		DatabaseHelper(Context context) {
			super(context, DATABASE_NAME, null, DATABASE_VERSION);
			mContext = context.getApplicationContext();
		}

		@Override
		public void onCreate(SQLiteDatabase db) {
			db.execSQL("CREATE TABLE " + Games.TABLE_NAME + " ("
					   + Games.TABLE_ID + " INTEGER PRIMARY KEY AUTOINCREMENT,"
					   + Games.KEY_GAMEID + " VARCHAR(255),"
					   + Games.KEY_TITLE + " VARCHAR(255),"
					   + Games.KEY_OVERVIEW + " VARCHAR(255),"
					   + Games.KEY_SERIAL + " VARCHAR(255),"
					   + Games.KEY_BOXART + " VARCHAR(255)"+ ");");
			
			String DATABASE_PATH = mContext.getFilesDir().getAbsolutePath() + "/../databases/";
			
			byte[] buffer = new byte[1024];
			OutputStream mOutput = null;
			int length;
			InputStream mInput = null;
			try
			{
				mInput = mContext.getAssets().open(DATABASE_NAME);
				mOutput = new FileOutputStream(DATABASE_PATH + DATABASE_NAME);
				while((length = mInput.read(buffer)) > 0)
				{
					mOutput.write(buffer, 0, length);
				}
				mOutput.close();
				mOutput.flush();
				mInput.close();
				
			}
			catch(IOException e)
			{
				e.printStackTrace();
			}
			
			db.execSQL("CREATE TABLE " + Covers.TABLE_NAME + " ("
					   + Covers.TABLE_ID + " INTEGER PRIMARY KEY AUTOINCREMENT,"
					   + Covers.KEY_SERIAL + " VARCHAR(255),"
					   + Covers.KEY_DISK + " VARCHAR(255),"
					   + Covers.KEY_IMAGE + " VARCHAR(255)"+ ");");
		}
		
		@Override
		public void onUpgrade(SQLiteDatabase db, int oldVersion, int newVersion) {
			Log.w("PlayDB", "Upgrading database from version " + oldVersion + " to "
						+ newVersion + ", which will destroy all old data");
			db.execSQL("DROP TABLE IF EXISTS " + Games.TABLE_NAME);
			db.execSQL("DROP TABLE IF EXISTS " + Covers.TABLE_NAME);
			onCreate(db);
		}
	}
	
	private DatabaseHelper dbHelper;

    /**
     * All CRUD(Create, Read, Update, Delete) Operations
     */

	@Override
	public int delete(Uri uri, String where, String[] whereArgs) {
		SQLiteDatabase db = dbHelper.getWritableDatabase();
		int count;
		switch (sUriMatcher.match(uri)) {
			case GAMES:
				count = db.delete(Games.TABLE_NAME, where, whereArgs);
				break;
			case GAMESID:
				count = db.delete(Games.TABLE_NAME, SQLITE_ID
							+ " = " + uri.getPathSegments().get(1)
							+ (!TextUtils.isEmpty(where) ? " AND ("
							+ where + ')' : ""), whereArgs);
			case COVERS:
				count = db.delete(Covers.TABLE_NAME, where, whereArgs);
				break;
			case COVERSID:
				count = db.delete(Covers.TABLE_NAME, SQLITE_ID
							+ " = " + uri.getPathSegments().get(1)
							+ (!TextUtils.isEmpty(where) ? " AND ("
							+ where + ')' : ""), whereArgs);
				break;
			default:
				throw new IllegalArgumentException("Unknown URI " + uri);
		}
		
		getContext().getContentResolver().notifyChange(uri, null);
		return count;
	}
	
	@Override
	public String getType(Uri uri) {
		switch (sUriMatcher.match(uri)) {
			case GAMES:
				return Games.CONTENT_TYPE;
			case GAMESID:
				return Games.CONTENT_TYPE_ID;
			case COVERS:
				return Covers.CONTENT_TYPE;
			case COVERSID:
				return Covers.CONTENT_TYPE_ID;
			default:
				throw new IllegalArgumentException("Unknown URI " + uri);
		}
	}
	
	@Override
	public Uri insert(Uri uri, ContentValues initialValues) {
		ContentValues values;
		if (initialValues != null) {
			values = new ContentValues(initialValues);
		} else {
			values = new ContentValues();
		}
		SQLiteDatabase db = dbHelper.getWritableDatabase();
		if (sUriMatcher.match(uri) == GAMES) {
			long rowId = db.insert(Games.TABLE_NAME, Games.KEY_GAMEID, values);
			if (rowId > 0) {
				Uri gameUri = ContentUris.withAppendedId(Games.GAMES_URI, rowId);
				getContext().getContentResolver().notifyChange(gameUri, null);
				return gameUri;
			}
			throw new SQLException("Failed to insert row into " + uri);
		}
		if (sUriMatcher.match(uri) == COVERS) {
			long rowId = db.insert(Covers.TABLE_NAME, Covers.KEY_SERIAL, values);
			if (rowId > 0) {
				Uri coverUri = ContentUris.withAppendedId(Covers.COVERS_URI, rowId);
				getContext().getContentResolver().notifyChange(coverUri, null);
				return coverUri;
			}
			throw new SQLException("Failed to insert row into " + uri);
		}
		throw new IllegalArgumentException("Unknown URI " + uri);
	}
	
	@Override
	public boolean onCreate() {
		dbHelper = new DatabaseHelper(getContext());
		return true;
	}
	
	@Override
	public Cursor query(Uri uri, String[] projection, String selection,
						String[] selectionArgs, String sortOrder) {
		SQLiteQueryBuilder qb = new SQLiteQueryBuilder();
		
		switch (sUriMatcher.match(uri)) {
			case GAMES:
				qb.setTables(Games.TABLE_NAME);
				qb.setProjectionMap(gamesMap);
				break;
			case COVERS:
				qb.setTables(Covers.TABLE_NAME);
				qb.setProjectionMap(coversMap);
				break;
			default:
				throw new IllegalArgumentException("Unknown URI " + uri);
		}
		
		SQLiteDatabase db = dbHelper.getReadableDatabase();
		Cursor c = qb.query(db, projection, selection, selectionArgs, null,
							null, sortOrder);
		
		c.setNotificationUri(getContext().getContentResolver(), uri);
		return c;
	}
	
	@Override
	public int update(Uri uri, ContentValues values, String where,
					  String[] whereArgs) {
		SQLiteDatabase db = dbHelper.getWritableDatabase();
		int count;
		switch (sUriMatcher.match(uri)) {
			case GAMES:
				count = db.update(Games.TABLE_NAME, values, where, whereArgs);
				break;
			case GAMESID:
				count = db.update(Games.TABLE_NAME, values,
							SQLITE_ID + " = "
							+ uri.getPathSegments().get(1)
							+ (!TextUtils.isEmpty(where) ? " AND ("
							+ where + ')' : ""), whereArgs);
			case COVERS:
				count = db.update(Covers.TABLE_NAME, values, where, whereArgs);
				break;
			case COVERSID:
				count = db.update(Covers.TABLE_NAME, values,
							SQLITE_ID + " = "
							+ uri.getPathSegments().get(1)
							+ (!TextUtils.isEmpty(where) ? " AND ("
							+ where + ')' : ""), whereArgs);
				break;
			default:
				throw new IllegalArgumentException("Unknown URI " + uri);
		}
		
		getContext().getContentResolver().notifyChange(uri, null);
		return count;
	}
	
	static {
		sUriMatcher = new UriMatcher(UriMatcher.NO_MATCH);
		
		sUriMatcher.addURI(Games.AUTHORITY, Games.TABLE_NAME, GAMES);
		
		gamesMap = new HashMap<String, String>();
		gamesMap.put(Games.TABLE_ID, Games.TABLE_ID);
		gamesMap.put(Games.KEY_GAMEID, Games.KEY_GAMEID);
		gamesMap.put(Games.KEY_TITLE, Games.KEY_TITLE);
		gamesMap.put(Games.KEY_OVERVIEW, Games.KEY_OVERVIEW);
		gamesMap.put(Games.KEY_SERIAL, Games.KEY_SERIAL);
		gamesMap.put(Games.KEY_BOXART, Games.KEY_BOXART);
		
		sUriMatcher.addURI(Games.AUTHORITY, Games.TABLE_NAME + "/#", GAMESID);
		
		sUriMatcher.addURI(Covers.AUTHORITY, Covers.TABLE_NAME, COVERS);
		
		coversMap = new HashMap<String, String>();
		coversMap.put(Covers.TABLE_ID, Covers.TABLE_ID);
		coversMap.put(Covers.KEY_SERIAL, Covers.KEY_SERIAL);
		coversMap.put(Covers.KEY_DISK, Covers.KEY_DISK);
		coversMap.put(Covers.KEY_IMAGE, Covers.KEY_IMAGE);
		
		sUriMatcher.addURI(Covers.AUTHORITY, Covers.TABLE_NAME + "/#", COVERSID);
		
	}

}
