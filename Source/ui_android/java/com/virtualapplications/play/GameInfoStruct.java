package com.virtualapplications.play;


import android.graphics.Bitmap;

import java.io.File;
import java.util.Comparator;

public class GameInfoStruct {
    private String m_TitleName;
    private String m_ID;
    //private final String m_Serial;
    private String m_FrontLink;
    private Bitmap m_Thumb;
    private Bitmap m_Thumb2;
    private String m_Overview;
    private File m_File;
    private String m_indexID;
    private long m_lastplayed;

    public GameInfoStruct(String m_id, String m_titlename, String overview, String m_frontLink) {
        m_TitleName = m_titlename;
        m_ID = m_id;
        m_Overview = overview;
        m_FrontLink = m_frontLink;
    }

    public GameInfoStruct(File file) {
        m_File = file;
        m_TitleName = file.getName();

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
    public String getID(){
        return m_ID;
    }
    /*public String getSerial(){
        return m_Serial;
    }*/

    public String getFrontLink() {
        return m_FrontLink;
    }

    public void setFrontCover(Bitmap thumb) {
        m_Thumb = thumb;
    }
    public void setBackCover(Bitmap thumb) { m_Thumb2 = thumb; }

    public Bitmap getFrontCover() {
        return m_Thumb;
    }

    public Bitmap getBackCover() {
        if (m_Thumb2 ==  null) {
            if (m_Thumb != null) {
                return m_Thumb;
            } else {
                return null;
            }

        } else {
            return m_Thumb2;
        }
    }

    public void setFile(File file) {
        m_File = file;
    }

    public File getFile() {
        return m_File;
    }

    public void setDescription(String overview) {
        m_Overview = overview;
    }

    public String getDescription() {
        if (m_Overview == null) {
            return "No info";
        } else {
            return m_Overview;
        }
    }

    public void setIndexID(String indexID) {
        this.m_indexID = indexID;
    }
    public void setlastplayed(long lastplayed) {
        this.m_lastplayed = lastplayed;
    }

    public long getlastplayed() {
        return m_lastplayed;
    }

    public static class GameInfoStructComparator implements Comparator<GameInfoStruct> {
        @Override
        public int compare(GameInfoStruct o1, GameInfoStruct o2) {

            return o1.getlastplayed() < o2.getlastplayed() ? -1 : o1.getlastplayed() == o2.getlastplayed() ? 0 : 1;
        }
    }
}