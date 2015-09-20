package com.virtualapplications.play.database;

import android.content.ContentValues;
import android.content.Context;
import android.database.Cursor;
import android.database.sqlite.SQLiteDatabase;
import android.database.sqlite.SQLiteOpenHelper;

import com.virtualapplications.play.GameInfoStruct;
import com.virtualapplications.play.MainActivity;

import java.io.File;
import java.util.ArrayList;
import java.util.List;

public class IndexingDB extends SQLiteOpenHelper {

    // All Static variables
    // Database Version
    private static final int DATABASE_VERSION = 2;

    // Database Name
    private static final String DATABASE_NAME = "GameIndex.db";


    public static final String KEY_ID = "id";
    public static final String KEY_DISKNAME = "DiskName";
    public static final String KEY_GAMETITLE = "GameTitle";
    public static final String KEY_PATH = "Path";
    public static final String KEY_GAMEID = "GameID";
    public static final String KEY_OVERVIEW = "Overview";
    public static final String KEY_SERIAL = "Serial";
    public static final String KEY_IMAGE = "Image";
    public static final String KEY_SIZE = "Size";
    public static final String KEY_LAST_PLAYED = "Last_Played";

    private static final String TABLE_NAME = "Games";

    public IndexingDB(Context context) {
        super(context, DATABASE_NAME, null, DATABASE_VERSION);
    }

    // Creating Tables
    @Override
    public void onCreate(SQLiteDatabase db) {
        String CREATE_STOPS_TABLE = "CREATE TABLE IF NOT EXISTS " + TABLE_NAME +
                "("
                + KEY_ID + " INTEGER PRIMARY KEY,"
                + KEY_DISKNAME + " TEXT,"
                + KEY_PATH + " TEXT,"
                + KEY_GAMEID + " INTEGER,"
                + KEY_GAMETITLE + " TEXT,"
                + KEY_OVERVIEW + " TEXT,"
                + KEY_SERIAL + " TEXT,"
                + KEY_IMAGE + " TEXT,"
                + KEY_SIZE + " TEXT,"
                + KEY_LAST_PLAYED + " INTEGER"
                + ")";
        db.execSQL(CREATE_STOPS_TABLE);
    }

    // Upgrading database
    @Override
    public void onUpgrade(SQLiteDatabase db, int oldVersion, int newVersion) {
        // Drop older table if existed
        db.execSQL("DROP TABLE IF EXISTS " + TABLE_NAME);

        // Create tables again
        onCreate(db);
    }

    /**
     * All CRUD(Create, Read, Update, Delete) Operations
     */

    void insert(ContentValues values) {
        SQLiteDatabase db = this.getWritableDatabase();

        // Inserting Row
        db.insert(TABLE_NAME, null, values);
        db.close(); // Closing database connection
    }

    void insertAll(List<ContentValues> values) {
        SQLiteDatabase db = this.getWritableDatabase();
        db.beginTransaction();
        // Inserting Row
        for (ContentValues value : values) {
            db.insert(TABLE_NAME, null, value);
        }
        db.setTransactionSuccessful();
        db.endTransaction();
        db.close(); // Closing database connection
    }

    ContentValues getGame(String selection, String[] selectionArgs) {
        SQLiteDatabase db = this.getReadableDatabase();

        Cursor cursor = db.query(TABLE_NAME, null, selection,
                selectionArgs, null, null, null, null);
        ContentValues values = new ContentValues();
        if (cursor != null){
            cursor.moveToFirst();

            for (String col : cursor.getColumnNames()){
                values.put(col, cursor.getString(cursor.getColumnIndex(col)));
            }

            cursor.close();
        }
        return values;
    }

    boolean isIndexed(String selection, String[] selectionArgs) {
        SQLiteDatabase db = this.getReadableDatabase();

        Cursor cursor = db.query(TABLE_NAME, null, selection,
                selectionArgs, null, null, null, null);
        boolean isGame = false;
        if (cursor != null){
            if (cursor.getCount() > 0)
                isGame = true;

            cursor.close();
        }

        return isGame;
    }

    public List<ContentValues> getIndexList(int sortMethod) {
        SQLiteDatabase db = this.getReadableDatabase();
        List<ContentValues> IndexList = new ArrayList<>();
        Cursor cursor;
        if (sortMethod == MainActivity.SORT_RECENT) {
            cursor = db.query(TABLE_NAME, null, KEY_LAST_PLAYED + " != 0", null, null, null, KEY_LAST_PLAYED + " DESC", null);
        } else if (sortMethod == MainActivity.SORT_HOMEBREW) {
            cursor = db.query(TABLE_NAME, null, KEY_DISKNAME + " LIKE ? COLLATE NOCASE", new String[]{"%.elf"}, null, null, null, null);
        } else {
            cursor = db.query(TABLE_NAME, null, null, null, null, null, null, null);
        }
        if (cursor != null){
            if (cursor.moveToFirst()) {
                do {
                    ContentValues values = new ContentValues();
                    for (String col : cursor.getColumnNames()){
                        values.put(col, cursor.getString(cursor.getColumnIndex(col)));
                    }
                    IndexList.add(values);
                } while (cursor.moveToNext());
            }
            cursor.close();
        }

        return IndexList;
    }

    public List<GameInfoStruct> getIndexGISList(int sortMethod) {
        SQLiteDatabase db = this.getReadableDatabase();
        List<GameInfoStruct> IndexList = new ArrayList<>();
        Cursor cursor;
        if (sortMethod == MainActivity.SORT_RECENT) {
            cursor = db.query(TABLE_NAME, null, KEY_LAST_PLAYED + " != 0", null, null, null, KEY_LAST_PLAYED + " DESC", null);
        } else if (sortMethod == MainActivity.SORT_HOMEBREW) {
            cursor = db.query(TABLE_NAME, null, KEY_DISKNAME + " LIKE ? COLLATE NOCASE", new String[]{"%.elf"}, null, null, null, null);
        } else {
            cursor = db.query(TABLE_NAME, null, null, null, null, null, null, null);
        }
        
        if (cursor != null) {
            if (cursor.moveToFirst()) {
                do {
                    String IndexID = cursor.getString(cursor.getColumnIndex(KEY_ID));
                    String GameID = cursor.getString(cursor.getColumnIndex(KEY_GAMEID));
                    String GameTitle = cursor.getString(cursor.getColumnIndex(KEY_GAMETITLE));
                    String FrontCoverLink = cursor.getString(cursor.getColumnIndex(KEY_IMAGE));
                    String OverView = cursor.getString(cursor.getColumnIndex(KEY_OVERVIEW));
                    String Path = cursor.getString(cursor.getColumnIndex(KEY_PATH));
                    String DiskName = cursor.getString(cursor.getColumnIndex(KEY_DISKNAME));
                    long last_played = cursor.getLong(cursor.getColumnIndex(KEY_LAST_PLAYED));
                    GameInfoStruct values = new GameInfoStruct(IndexID, GameID, GameTitle, OverView, FrontCoverLink, last_played, new File(Path, DiskName));
                    IndexList.add(values);
                } while (cursor.moveToNext());
            }
            cursor.close();
        }
        return IndexList;
    }

    public void updateIndex(ContentValues values, String selection, String[] selectionArgs) {
        SQLiteDatabase db = this.getWritableDatabase();

        // updating row
        db.update(TABLE_NAME, values, selection, selectionArgs);
        db.close();
    }

    public void deleteIndex(String selection, String[] selectionArgs) {
        SQLiteDatabase db = this.getWritableDatabase();
        db.delete(TABLE_NAME, selection, selectionArgs);
        db.close();
    }


    public int getIndexCount() {
        SQLiteDatabase db = this.getReadableDatabase();

        Cursor cursor = db.query(TABLE_NAME, null, null,
                null, null, null, null, null);
        int count = 0;
        if (cursor != null) {
            count = cursor.getCount();
            cursor.close();
        }
        // return count
        return count;
    }

    public List<String> getPaths() {
        SQLiteDatabase db = this.getReadableDatabase();

        Cursor cursor = db.query(true, TABLE_NAME, new String[]{KEY_PATH}, null, null, KEY_PATH, null, null, null);
        List<String> paths = new ArrayList<>();
        if (cursor != null) {
            if (cursor.moveToFirst()) {
                do {
                    paths.add(cursor.getString(cursor.getColumnIndex(KEY_PATH)));
                } while (cursor.moveToNext());
            }
            cursor.close();
        }

        return paths;
    }

    public void resetDatabase() {
        SQLiteDatabase db = this.getWritableDatabase();
        onUpgrade(db, 0, 0);
        db.close();
    }
}
