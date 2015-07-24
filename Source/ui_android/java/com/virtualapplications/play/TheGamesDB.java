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

import java.util.ArrayList;
import java.util.HashMap;
import java.util.List;

import com.virtualapplications.play.SqliteHelper.Games;

public class TheGamesDB extends ContentProvider {
	
	// Database Name
	private static final String DATABASE_NAME = "games.db";

    // Database Version
    private static final int DATABASE_VERSION = 1;
	
	public static final String SQLITE_ID = "_id";

	private static final UriMatcher sUriMatcher;
	
	private static final int GAMES = 1;
	private static final int GAMESID = 2;
	
	private static HashMap<String, String> gamesMap;
	
	private static class DatabaseHelper extends SQLiteOpenHelper {

		DatabaseHelper(Context context) {
			super(context, DATABASE_NAME, null, DATABASE_VERSION);
		}

		@Override
		public void onCreate(SQLiteDatabase db) {
			db.execSQL("CREATE TABLE IF NOT EXISTS " + Games.TABLE_NAME + " ("
					   + Games.TABLE_ID + " INTEGER PRIMARY KEY AUTOINCREMENT,"
					   + Games.KEY_GAMEID + " VARCHAR(255),"
					   + Games.KEY_TITLE + " VARCHAR(255),"
					   + Games.KEY_OVERVIEW + " VARCHAR(255),"
					   + Games.KEY_SERIAL + " VARCHAR(255),"
					   + Games.KEY_BOXART + " VARCHAR(255)"+ ");");
		}
		
		@Override
		public void onUpgrade(SQLiteDatabase db, int oldVersion, int newVersion) {
			Log.w("PlayDB", "Upgrading database from version " + oldVersion + " to "
						+ newVersion + ", which will destroy all old data");
			db.execSQL("DROP TABLE IF EXISTS " + Games.TABLE_NAME);
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
		
	}

}
