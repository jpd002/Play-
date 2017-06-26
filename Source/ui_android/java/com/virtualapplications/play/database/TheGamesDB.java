package com.virtualapplications.play.database;

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
import android.database.sqlite.SQLiteStatement;
import android.net.Uri;
import android.os.AsyncTask;
import android.os.Bundle;
import android.text.TextUtils;
import android.util.Log;

import java.io.File;
import java.io.FileOutputStream;
import java.io.InputStream;
import java.io.IOException;
import java.io.OutputStream;

import java.util.ArrayList;
import java.util.HashMap;
import java.util.List;

import com.virtualapplications.play.database.SqliteHelper.Games;
import com.virtualapplications.play.database.SqliteHelper.Covers;

public class TheGamesDB extends ContentProvider {

	// Database Name
	private static final String DATABASE_NAME = "games.db";

	// Database Version
	private static final int DATABASE_VERSION = 3;

	public static final String SQLITE_ID = "_id";

	private static final UriMatcher sUriMatcher;

	private static final int GAMES = 1;
	private static final int GAMESID = 2;
	private static final int COVERS = 3;
	private static final int COVERSID = 4;

	private static HashMap<String, String> gamesMap;
	private static HashMap<String, String> coversMap;

	private static Context mContext;

	private static class DatabaseHelper extends SQLiteOpenHelper {

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
					   + Games.KEY_SERIAL + " VARCHAR(255)"+ ");");

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
	public Bundle call(String method, String arg, Bundle extras) {
		SQLiteDatabase db = dbHelper.getWritableDatabase();
		if (method.equals("importDb")) {
			if (dbLength() == 0) {
				new importDb().execute();
			}
		}
		return null;
	}

	public int dbLength() {
		Cursor c = getContext().getContentResolver().query(Games.GAMES_URI, null, null, null, null);
		return c.getCount();
	}

	@Override
	public int bulkInsert(Uri uri, ContentValues values[]) {
		SQLiteDatabase db = dbHelper.getWritableDatabase();

		String TABLE_NAME;
		switch (sUriMatcher.match(uri)) {
			case GAMES:
			case GAMESID:
				TABLE_NAME = Games.TABLE_NAME;
				break;
			case COVERS:
			case COVERSID:
				throw new IllegalArgumentException("URI is not Implemented to work with bulkInsert() yet.");
			default:
				throw new IllegalArgumentException("Unknown URI " + uri);
		}

		db.beginTransaction();
		SQLiteStatement stmt = db.compileStatement("INSERT INTO "+ TABLE_NAME + "(" + Games.KEY_GAMEID +  "," + Games.KEY_TITLE +  "," +  Games.KEY_SERIAL + ") VALUES(?,?,?)");

		int numInserted = 0;
		try {
			int len = values.length;
			for (ContentValues value : values) {

				String KEY_GAMEID = (String)(value.get(Games.KEY_GAMEID));
				if (KEY_GAMEID != null) {
					stmt.bindString(1, KEY_GAMEID);
				}
				String KEY_TITLE = (String)(value.get(Games.KEY_TITLE));
				if (KEY_TITLE != null) {
					stmt.bindString(2, KEY_TITLE);
				}
				String KEY_SERIAL = (String)(value.get(Games.KEY_SERIAL));
				if (KEY_SERIAL != null) {
					stmt.bindString(3, KEY_SERIAL);
				}


				stmt.execute();
				stmt.clearBindings();
			}
			numInserted = len;
			db.setTransactionSuccessful();
		} finally {
			db.endTransaction();
			stmt.close();
		}
		getContext().getContentResolver().notifyChange(uri, null);
		return numInserted;
	}

	public class importDb extends AsyncTask<String, Integer, String> {

		private String DATABASE_PATH;

		protected void onPreExecute() {
			DATABASE_PATH = mContext.getFilesDir().getAbsolutePath();
			byte[] buffer = new byte[1024];
			OutputStream mOutput = null;
			int length;
			InputStream mInput = null;
			try
			{
				mInput = mContext.getAssets().open(DATABASE_NAME);
				mOutput = new FileOutputStream(DATABASE_PATH + "/" + DATABASE_NAME);
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
		}

		@Override
		protected String doInBackground(String... params) {
			try {
				SQLiteDatabase source = SQLiteDatabase.openDatabase(DATABASE_PATH
						+ "/" + DATABASE_NAME, null, SQLiteDatabase.OPEN_READWRITE);
				Cursor c = source.rawQuery("SELECT * FROM " + Games.TABLE_NAME, null);
				List<ContentValues> games = new ArrayList<>();
				if (c.moveToFirst()) {
					do {
						ContentValues game = new ContentValues();
						game.put(Games.KEY_GAMEID, c.getString(c.getColumnIndex(Games.KEY_GAMEID)));
						game.put(Games.KEY_TITLE, c.getString(c.getColumnIndex(Games.KEY_TITLE)));
						game.put(Games.KEY_SERIAL, c.getString(c.getColumnIndex(Games.KEY_SERIAL)));
						games.add(game);
					} while (c.moveToNext());
				}
				c.close();
				source.close();
				getContext().getContentResolver().bulkInsert(Games.GAMES_URI, games.toArray(new ContentValues[games.size()]));
			} catch (SQLException ex)  {
				ex.printStackTrace();
			}
			return null;
		}

		@Override
		protected void onPostExecute(String result) {
			File tempDb = new File (DATABASE_PATH, DATABASE_NAME);
			if (tempDb.exists()) {
				tempDb.delete();
			}
			File tempDbJournal = new File (DATABASE_PATH, DATABASE_NAME + "-journal");
			if (tempDbJournal.exists()) {
				tempDbJournal.delete();
			}
		}
	}

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
				break;
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
		gamesMap.put(Games.KEY_SERIAL, Games.KEY_SERIAL);

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
