package com.virtualapplications.play;


import android.content.ContentValues;
import android.content.Context;

import com.virtualapplications.play.database.IndexingDB;

import java.io.File;

public class GameInfoStruct {
    private String m_Serial;
    private String m_TitleName;
    private String m_ID;
    private String m_FrontLink;
    private String m_Overview;
    private File m_File;
    private final String m_indexID;
    private long m_lastplayed;

    public GameInfoStruct(String indexid, String m_id, String m_titlename, String overview, String m_frontLink, long last_played, File file, String serial) {
        m_TitleName = m_titlename;
        m_ID = m_id;
        m_Overview = overview;
        m_FrontLink = m_frontLink;
        m_indexID = indexid;
        m_lastplayed = last_played;
        m_File = file;
        m_Serial = serial;
    }

    public void setTitleName(String title, Context mContext){
        m_TitleName = title;
        if (mContext != null){
            IndexingDB GI = new IndexingDB(mContext);

            ContentValues values = new ContentValues();
            values.put(IndexingDB.KEY_GAMETITLE, title);
            GI.updateIndex(values, IndexingDB.KEY_ID + "=?", new String[]{m_indexID});
        }
    }

    public String getTitleName(){
        if (m_TitleName != null){
            return m_TitleName;
        } else {
            if (m_File != null)
                return m_File.getName();
        }
        return null;
    }

    public boolean isTitleNameEmptyNull(){
        return m_TitleName == null || m_TitleName.isEmpty();
    }

    //If this method is called, then IndexDB is missing GameID, so update it to reflect that
    public void setGameID(String id, Context mContext) {
        this.m_ID = id;
        if (mContext != null){
            IndexingDB GI = new IndexingDB(mContext);

            ContentValues values = new ContentValues();
            values.put(IndexingDB.KEY_GAMEID, id);
            GI.updateIndex(values, IndexingDB.KEY_ID + "=?", new String[]{m_indexID});
            GI.close();
        }
    }

    public String getGameID(){
        return m_ID;
    }

    public void setFrontLink(String frontLink, Context mContext) {
        this.m_FrontLink = frontLink;
        if (mContext != null){
            IndexingDB GI = new IndexingDB(mContext);

            ContentValues values = new ContentValues();
            values.put(IndexingDB.KEY_IMAGE, frontLink);
            GI.updateIndex(values, IndexingDB.KEY_ID + "=?", new String[]{m_indexID});
            GI.close();
        }
    }

    public String getFrontLink() {
        return m_FrontLink;
    }

    public void setFile(File file) {
        m_File = file;
    }

    public File getFile() {
        return m_File;
    }

    public void setDescription(String overview, Context mContext) {
        m_Overview = overview;
        if (mContext != null){
            IndexingDB GI = new IndexingDB(mContext);

            ContentValues values = new ContentValues();
            values.put(IndexingDB.KEY_OVERVIEW, overview);
            GI.updateIndex(values, IndexingDB.KEY_ID + "=?", new String[]{m_indexID});
            GI.close();
        }
    }

    public String getDescription() {
        if (m_Overview == null) {
            return "No info";
        } else {
            return m_Overview;
        }
    }

    public boolean isDescriptionEmptyNull(){
        return m_Overview == null || m_Overview.isEmpty();
    }

    public void setlastplayed(Context mContext) {
        this.m_lastplayed = System.currentTimeMillis();
        if (mContext != null){
            IndexingDB GI = new IndexingDB(mContext);

            ContentValues values = new ContentValues();
            values.put(IndexingDB.KEY_LAST_PLAYED, System.currentTimeMillis());
            GI.updateIndex(values, IndexingDB.KEY_ID + "=?", new String[]{m_indexID});
            GI.close();
        }
    }

    public long getlastplayed() {
        return m_lastplayed;
    }

    public String getIndexID() {
        return m_indexID;
    }

    public void removeIndex(Context mContext) {
        if (mContext != null){
            IndexingDB GI = new IndexingDB(mContext);

            GI.deleteIndex(IndexingDB.KEY_ID + "=?", new String[]{m_indexID});
            GI.close();
        }
    }

    public String getSerial() {
        return m_Serial;
    }
}