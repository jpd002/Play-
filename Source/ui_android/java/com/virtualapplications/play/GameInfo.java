package com.virtualapplications.play;

import android.content.Context;
import android.graphics.Bitmap;
import android.os.AsyncTask;

import java.util.concurrent.ExecutionException;

/**
 * Created by alawi on 7/19/15.
 */
public class GameInfo {
    private final String m_Name;
    private final int m_ID;
    private final String m_Serial;
    private final String m_FrontLink;
    private Bitmap m_Thumb;
    private Bitmap m_Thumb2;
    private String m_Details;

    public GameInfo(int m_id, String m_name, String m_serial, String m_frontLink) {
        m_Name = m_name;
        m_ID = m_id;
        m_Serial = m_serial;
        m_FrontLink = m_frontLink;
    }
    public String getName(){
        return m_Name;
    }
    public int getID(){
        return m_ID;
    }
    public String getSerial(){
        return m_Serial;
    }

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

    public void setDescription(String details) {
        m_Details = details;
    }

    public String getDescription(final Context mContext) {
        if (m_Details == null) {
            AsyncTask<String,Integer,String> aTask1;
            aTask1 = new AsyncTask<String, Integer, String>() {
                @Override
                protected String doInBackground(String... params) {

                    return GamesDbAPI.getDescription(m_Name, m_ID, mContext);
                }
            };
            try {
                m_Details = aTask1.execute().get();
            } catch (InterruptedException e) {
                e.printStackTrace();
            } catch (ExecutionException e) {
                e.printStackTrace();
            }
            if (this.getID() != 0 & m_Details != null){
                TheGameDB db = new TheGameDB(mContext);
                db.updateGame(this, m_Details);
            }
        }
        if (m_Details == null) {
            return "No info";
        } else {
            return m_Details;
        }
    }
}
