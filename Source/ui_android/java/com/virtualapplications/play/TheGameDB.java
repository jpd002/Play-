package com.virtualapplications.play;

import android.content.ContentValues;
import android.content.Context;
import android.database.Cursor;
import android.database.sqlite.SQLiteDatabase;
import android.database.sqlite.SQLiteOpenHelper;

import java.io.File;
import java.util.ArrayList;
import java.util.List;


public class TheGameDB extends SQLiteOpenHelper {

    // All Static variables
    // Database Version
    private static final int DATABASE_VERSION = 1;

    // Database Name
    private static final String DATABASE_NAME = "thegame.db";

    // Table name
    private static final String TABLE_NAME = "gamedb";

    // Contacts Table Columns names
    //private static final String KEY_ID = "id";
    private static final String KEY_ID = "id";
    private static final String KEY_Name = "name";
    private static final String KEY_Serial = "code";
    private static final String KEY_Front = "Front";
    private static final String KEY_Desc = "desc";
    //private static final String KEY_Back = "Back";

    public TheGameDB(Context context) {
        super(context, DATABASE_NAME, null, DATABASE_VERSION);
    }

    // Creating Tables
    @Override
    public void onCreate(SQLiteDatabase db) {
        String CREATE_STOPS_TABLE = "CREATE TABLE IF NOT EXISTS " + TABLE_NAME + "("
                + KEY_ID + " TEXT," + KEY_Name + " TEXT,"
                + KEY_Serial + " TEXT," + KEY_Desc + " TEXT," + KEY_Front + " TEXT"
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


    void addGame(GameInfo Game) {
        SQLiteDatabase db = this.getWritableDatabase();

        ContentValues values = new ContentValues();
        values.put(KEY_ID, Game.getID());
        values.put(KEY_Name, Game.getName());
        values.put(KEY_Serial, Game.getSerial());
        values.put(KEY_Front, Game.getFrontLink()); // not sure why I've left this here, since we still need to request the description and it includes both cover links

        // Inserting Row
        db.insert(TABLE_NAME, null, values);
        db.close(); // Closing database connection
    }

    GameInfo getGame(int id) {
        SQLiteDatabase db = this.getReadableDatabase();

        Cursor cursor = db.query(TABLE_NAME, new String[]{KEY_ID,
                        KEY_Name, KEY_Serial}, KEY_ID + "=?",
                new String[]{String.valueOf(id)}, null, null, null, null);
        if (cursor != null)
            cursor.moveToFirst();

        GameInfo stop = new GameInfo(Integer.parseInt(cursor.getString(0)),
                cursor.getString(1), cursor.getString(2), null);
        cursor.close();
        return stop;
    }

    GameInfo getGame(String serial, String name) {
        SQLiteDatabase db= null;
        db = this.getReadableDatabase();
        Cursor cursor = db.query(TABLE_NAME, new String[]{KEY_ID,
                        KEY_Name, KEY_Serial},
                KEY_Serial + " LIKE ? COLLATE NOCASE",
                new String[]{"%"+serial+"%"}, null, null, null, null);
        if (cursor != null)
            cursor.moveToFirst();

        if (cursor.getCount()>0){
        GameInfo game = new GameInfo(Integer.parseInt(cursor.getString(0)),
                cursor.getString(1), cursor.getString(2), null);
            cursor.close();
            return game;
        } else {
            cursor.close();
            return new GameInfo(0, name, serial, null);
        }

    }

    boolean isGame(int id) {
        SQLiteDatabase db = this.getReadableDatabase();

        Cursor cursor = db.query(TABLE_NAME, new String[]{KEY_ID,
                        KEY_Name, KEY_Serial}, KEY_ID + "=? COLLATE NOCASE",
                new String[]{String.valueOf(id)}, null, null, null, null);

        if (cursor != null)
            cursor.moveToFirst();

        if (cursor.getCount() > 0) {
            cursor.close();
            return true;
        }

        cursor.close();
        return false;
    }

    int isGame(String name) {
        SQLiteDatabase db = this.getReadableDatabase();

        Cursor cursor = db.query(TABLE_NAME, new String[]{KEY_ID,
                        KEY_Name, KEY_Serial, KEY_Front}, KEY_Name + "=? COLLATE NOCASE",
                new String[]{name}, null, null, null, null);

        if (cursor != null)
            cursor.moveToFirst();

        if (cursor.getCount() > 0) {
            int id = Integer.parseInt(cursor.getString(0));
            cursor.close();
            return id;
        } else {

            cursor.close();
            return 0;
        }

    }

    public List<GameInfo> getAllGames() {
        List<GameInfo> stopList = new ArrayList<GameInfo>();
        // Select All Query
        String selectQuery = "SELECT  * FROM " + TABLE_NAME;

        SQLiteDatabase db = this.getWritableDatabase();
        Cursor cursor = db.rawQuery(selectQuery, null);

        // looping through all rows and adding to list
        if (cursor.moveToFirst()) {
            do {
                GameInfo stop = new GameInfo(Integer.parseInt(cursor.getString(0)),
                        cursor.getString(1), cursor.getString(2), cursor.getString(3));
                stopList.add(stop);
            } while (cursor.moveToNext());
        }

        cursor.close();
        return stopList;
    }


    public int updateGame(GameInfo Game, String Desc) {
        SQLiteDatabase db = this.getWritableDatabase();

        ContentValues values = new ContentValues();
        values.put(KEY_ID, Game.getID());
        values.put(KEY_Name, Game.getName());
        values.put(KEY_Serial, Game.getSerial());
        values.put(KEY_Front, Game.getFrontLink());
        values.put(KEY_Desc, Desc);

        // updating row
        return db.update(TABLE_NAME, values, KEY_ID + " = ?",
                new String[]{String.valueOf(Game.getID())});
    }

    public int updateGameSerial(GameInfo game, int ID) {
        String serial = getGame(ID).getSerial();
        if (serial != null && serial.length()>0){
            serial = serial+",";
        } else {
            serial ="";
        }
        SQLiteDatabase db = this.getWritableDatabase();
        ContentValues values = new ContentValues();
        values.put(KEY_Serial, serial + game.getSerial());

        // updating row
        return db.update(TABLE_NAME, values, KEY_Name + " = ? COLLATE NOCASE",
                new String[]{game.getName()});
    }

    public void deleteGame(GameInfo Game) {
        SQLiteDatabase db = this.getWritableDatabase();
        db.delete(TABLE_NAME, KEY_ID + " = ?",
                new String[]{String.valueOf(Game.getID())});
        db.close();
    }


    public int getGameCount() {
        String countQuery = "SELECT  * FROM " + TABLE_NAME;
        SQLiteDatabase db = this.getReadableDatabase();
        Cursor cursor = db.rawQuery(countQuery, null);
        int count = cursor.getCount();
        cursor.close();

        // return count
        return count;
    }

    public static boolean isTableExists( Context mContext) {
        File file = new File(String.valueOf(mContext.getDatabasePath(DATABASE_NAME)));
        if (!file.exists()) {
            TheGameDB db = new TheGameDB(mContext);
            db.getReadableDatabase();
            db.close();
            return false;
        } else {
            return true;
        }
    }

}
