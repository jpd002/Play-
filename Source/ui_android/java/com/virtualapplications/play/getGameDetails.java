package com.virtualapplications.play;

import android.content.Context;
import android.graphics.Bitmap;
import android.graphics.BitmapFactory;
import android.net.ConnectivityManager;
import android.net.NetworkInfo;
import android.os.AsyncTask;
import android.os.Build;
import android.os.StrictMode;
import android.util.SparseArray;
import android.view.View;
import android.widget.ImageView;
import android.widget.TextView;

import java.io.File;
import java.io.FileNotFoundException;
import java.io.FileOutputStream;
import java.io.IOException;;
import java.io.OutputStream;

public class getGameDetails extends AsyncTask<String, Integer, GameInfo> {

    private final File game;
    private final int index;
    private Context mContext;
    private View childview;


    private ImageView preview;
    private SparseArray<String> game_details = new SparseArray<String>();
    public GameInfo GameDetails;

    protected void onPreExecute() {
        preview = (ImageView) childview.findViewById(R.id.game_icon);
        if (Build.VERSION.SDK_INT >= Build.VERSION_CODES.GINGERBREAD) {
            StrictMode.ThreadPolicy policy = new StrictMode.ThreadPolicy.Builder()
                    .permitAll().build();
            StrictMode.setThreadPolicy(policy);
        }
    }

    public GameInfo getGameInfo(File game) {
        TheGameDB db = new TheGameDB(MainActivity.mActivity);
        String Serial = getSerial(game);
        GameInfo TheGame = db.getGame(Serial, game.getName());
        int ID = TheGame.getID();
        Bitmap bitmap = null;
            if (ID != 0) {
                bitmap = getImage(String.valueOf(ID));
            }
            if (bitmap != null) {
                TheGame.setThumb(bitmap);
            } else {
                try {
                    if (isNetworkAvailable()){
                        GamesDbAPI GDB = new GamesDbAPI();
                        TheGame = GDB.getCover(TheGame, index, mContext, preview);
                        /*//TODO:call GameDB to get cover, save cover and set in game
                        doc = getDomElement(GamesDbAPI_xml(TheGame.getName(), ID));
                        if (doc.getElementsByTagName("Game") != null) {
                            Element root = (Element) doc.getElementsByTagName("Game").item(0);
                            game_name = getValue(root, "GameTitle");
                            String details = getValue(root, "Overview");
                            game_details.put(index, details);
                            Element images = (Element) root.getElementsByTagName("Images").item(0);
                            Element sleave = (Element) images.getElementsByTagName("boxart").item(0);
                            String cover = "http://thegamesdb.net/banners/" + getElementValue(sleave);
                            game_icon = new BitmapDrawable(decodeBitmapIcon(cover));
                            Element boxart = (Element) images.getElementsByTagName("boxart").item(1);
                            String image = "http://thegamesdb.net/banners/" + getElementValue(boxart);
                            Bitmap game_image = decodeBitmapIcon(image);
                            TheGame.setThumb(game_image);
                            game_preview.put(index, TheGame.getBitmap());*/
                            if (TheGame.getID() != 0) {
                                if (TheGame.getBitmap() != null)
                                    saveimage(String.valueOf(ID), TheGame.getBitmap());
                            }
                        /*} else {
                            game_details.put(index, mContext.getString(R.string.game_info));
                        }*/
                    }
                } catch (Exception e) {
                    game_details.put(index, mContext.getString(R.string.game_info));
                    e.printStackTrace();
                }

                //addBitmapToMemoryCache(String.valueOf(ID), bitmap);
            }
        return TheGame;
    }

    private Bitmap getImage(String s) {
        String path = mContext.getFilesDir() + "/covers/";

        File file = new File(path, s+".jpg");
        if(file.exists())
        {
            BitmapFactory.Options options = new BitmapFactory.Options();
            options.inPreferredConfig = Bitmap.Config.ARGB_8888;
            Bitmap bitmap = BitmapFactory.decodeFile(file.getAbsolutePath(), options);
            return bitmap;
        } else {
            return null;
        }
    }



    public boolean isNetworkAvailable() {
        ConnectivityManager connectivityManager = (ConnectivityManager) mContext
                .getSystemService(Context.CONNECTIVITY_SERVICE);
        NetworkInfo mWifi = connectivityManager.getNetworkInfo(ConnectivityManager.TYPE_WIFI);
        NetworkInfo mMobile = connectivityManager.getNetworkInfo(ConnectivityManager.TYPE_MOBILE);
        NetworkInfo activeNetworkInfo = connectivityManager.getActiveNetworkInfo();
        if (mMobile != null && mWifi != null) {
            return mMobile.isAvailable() || mWifi.isAvailable();
        } else {
            return activeNetworkInfo != null && activeNetworkInfo.isConnected();
        }
    }



    String[] SerialDummy = new String[]{"SCUS_974.81", "SLPM_661.51", "SLUS_210.05", "SLPS_250.50", "SLUS_209.63", "SCUS_974.72"};
    public static int tmpint = -1;

    public String getSerial(File game) {
        //TODO:A Call To Native Call
        if (tmpint >= 5) tmpint=-1;
        return SerialDummy[++tmpint];
    }



    private void saveimage(String key, Bitmap image){
        String path = mContext.getFilesDir() + "/covers/";
        OutputStream fOut = null;
        File file = new File(path, key+".jpg"); // the File to save to
        if (!file.getParentFile().exists()) {
            file.getParentFile().mkdir();
        }
        try {
            fOut = new FileOutputStream(file, false);
            image.compress(Bitmap.CompressFormat.JPEG, 85, fOut); // saving the Bitmap to a file compressed as a JPEG with 85% compression rate
            fOut.flush();
            fOut.close(); // do not forget to close the stream
        } catch (FileNotFoundException e) {
            e.printStackTrace();
        } catch (IOException e) {
            e.printStackTrace();
        }

    }


    public getGameDetails(File game, int index) {
        this.game = game;
        this.index = index;
    }

    public void setViewParent(Context mContext, View childview) {
        this.mContext = mContext;
        this.childview = childview;
    }

    @Override
    protected GameInfo doInBackground(String... params) {
        GameDetails = getGameInfo(game);
        return GameDetails;
    }

    protected void onPostExecute(GameInfo gameData) {

            if (gameData.getBitmap() != null){
                preview.setImageBitmap(gameData.getBitmap());
                preview.setScaleType(ImageView.ScaleType.CENTER_INSIDE);
                ((TextView) childview.findViewById(R.id.game_text)).setVisibility(View.GONE);
            } else {
                ((TextView) childview.findViewById(R.id.game_text)).setText(gameData.getName());
            }
            


        //     ((TextView) childview.findViewById(R.id.game_text)).setText(game.getName());

    }
}