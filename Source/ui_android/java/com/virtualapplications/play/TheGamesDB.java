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

public class TheGamesDB extends ContentProvider {

    // All Static variables
	public static final String AUTHORITY = "virtualapplications.play.gamesdb";
	
	public static final String CONTENT_TYPE = "vnd.android.cursor.dir/vnd.virtualapplications.play";
	public static final String CONTENT_TYPE_ID = "vnd.android.cursor.item/vnd.virtualapplications.play";
	
    // Database Version
    private static final int DATABASE_VERSION = 1;
	
	public static final String SQLITE_ID = "_id";

    // Database Name
    private static final String DATABASE_NAME = "games.db";

    // Contacts table name
    private static final String TABLE_NAME = "games";
	
	public static final Uri GAMESDB_URI = Uri.parse("content://" + AUTHORITY + "/" + TABLE_NAME);
	
	private static final UriMatcher sUriMatcher;
	
	private static final int GAMES = 1;
	private static final int GAMESID = 2;

    // Contacts Table Columns names
	public static final String KEY_GAMEID = "GameID";
    public static final String KEY_TITLE = "GameTitle";
	public static final String KEY_OVERVIEW = "Overview";
    public static final String KEY_SERIAL = "Serial";
	
	private static HashMap<String, String> gamesMap;
	
	private static class DatabaseHelper extends SQLiteOpenHelper {

		DatabaseHelper(Context context) {
			super(context, DATABASE_NAME, null, DATABASE_VERSION);
		}

		@Override
		public void onCreate(SQLiteDatabase db) {
			db.execSQL("CREATE TABLE " + TABLE_NAME + " ("
					   + SQLITE_ID + " INTEGER PRIMARY KEY AUTOINCREMENT,"
					   + KEY_GAMEID + " VARCHAR(255),"
					   + KEY_TITLE + " VARCHAR(255),"
					   + KEY_OVERVIEW + " VARCHAR(255)"
					   + KEY_SERIAL + " VARCHAR(255)," + ");");
		}
		
		@Override
		public void onUpgrade(SQLiteDatabase db, int oldVersion, int newVersion) {
			Log.w("PlayDB", "Upgrading database from version " + oldVersion + " to "
						+ newVersion + ", which will destroy all old data");
			db.execSQL("DROP TABLE IF EXISTS " + TABLE_NAME);
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
				count = db.delete(TABLE_NAME, where, whereArgs);
				break;
			case GAMESID:
				count = db.delete(TABLE_NAME, SQLITE_ID
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
				return CONTENT_TYPE;
			case GAMESID:
				return CONTENT_TYPE_ID;
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
			long rowId = db.insert(TABLE_NAME, KEY_GAMEID, values);
			if (rowId > 0) {
				Uri gameUri = ContentUris.withAppendedId(GAMESDB_URI, rowId);
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
				qb.setTables(TABLE_NAME);
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
				count = db.update(TABLE_NAME, values, where, whereArgs);
				break;
			case GAMESID:
				count = db.update(TABLE_NAME, values,
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
		
		sUriMatcher.addURI(AUTHORITY, TABLE_NAME, GAMES);
		
		gamesMap = new HashMap<String, String>();
		gamesMap.put(SQLITE_ID, SQLITE_ID);
		gamesMap.put(KEY_GAMEID, KEY_GAMEID);
		gamesMap.put(KEY_TITLE, KEY_TITLE);
		gamesMap.put(KEY_OVERVIEW, KEY_OVERVIEW);
		gamesMap.put(KEY_SERIAL, KEY_SERIAL);
		
		sUriMatcher.addURI(AUTHORITY, TABLE_NAME + "/#", GAMESID);
		
	}

}
