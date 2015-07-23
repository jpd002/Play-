package com.virtualapplications.play;

import android.content.Context;
import android.database.Cursor;
import android.database.sqlite.SQLiteDatabase;
import android.os.AsyncTask;

import java.io.FileOutputStream;
import java.io.IOException;
import java.io.InputStream;
import java.io.OutputStream;
import java.util.ArrayList;
import java.util.List;

;import static com.virtualapplications.play.GamesDbAPI.getGameDBList;
import static com.virtualapplications.play.TheGameDB.isTableExists;

//Download thegameDB + parse to sql + CrossMatch With SkyNet DB
public class SetupDB extends AsyncTask<Context, Integer, String> {

    @Override
    protected String doInBackground(Context... params) {
        if (!isTableExists(params[0])){
            getGameDBList(params[0]);
            if (isTableExists(params[0])) {
                copyDataBase(params[0]);
                List<GameInfo> list = getSkynetDB(params[0]);
                if (list.size() > 3){
                    crossRefernce(list, params[0]);
                }
            }
        }
        return null;
    }

    private void copyDataBase(Context mContext) {
        byte[] buffer = new byte[1024];
        OutputStream myOutput = null;
        int length;
        // Open your local db as the input stream
        InputStream myInput = null;
        try
        {
            myInput = mContext.getAssets().open("skynet.db");
            // transfer bytes from the inputfile to the outputfile
            myOutput =new FileOutputStream(mContext.getDatabasePath("skynet.db"));
            while((length = myInput.read(buffer)) > 0)
            {
                myOutput.write(buffer, 0, length);
            }
            myOutput.close();
            myOutput.flush();
            myInput.close();


        }
        catch(IOException e)
        {
            e.printStackTrace();
        }
    }

    private List<GameInfo> crossRefernce(List<GameInfo> param, Context mContext) {
        TheGameDB db = new TheGameDB(mContext);
        for (GameInfo game : param){
            int ID = db.isGame(game.getName());
            if (ID > 0){
                db.updateGameSerial(game,ID);
            }
        }
        return null;
    }

    private List<GameInfo> getSkynetDB(Context mContext) {
        List<GameInfo> stopList = new ArrayList<GameInfo>();
        // Select All Query
        String selectQuery = "SELECT  * FROM " + "gamedb";

        SQLiteDatabase db = SQLiteDatabase.openDatabase(mContext.getDatabasePath("skynet.db").getAbsolutePath(), null, 1);
        Cursor cursor = db.rawQuery(selectQuery, null);

        // looping through all rows and adding to list
        if (cursor.moveToFirst()) {
            do {
                GameInfo stop = new GameInfo(0,
                        cursor.getString(0), cursor.getString(1), null);
                stopList.add(stop);
            } while (cursor.moveToNext());
        }

        cursor.close();
        return stopList;
    }



};