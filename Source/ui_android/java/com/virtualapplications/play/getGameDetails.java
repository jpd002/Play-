package com.virtualapplications.play;

import android.app.Activity;
import android.content.Context;
import android.graphics.Bitmap;
import android.graphics.BitmapFactory;
import android.net.ConnectivityManager;
import android.net.NetworkInfo;
import android.os.AsyncTask;
import android.os.Build;
import android.os.StrictMode;
import android.util.Log;
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
    private Context mContext;
    private View childview;


    private ImageView preview;
    public GameInfo GameInfo;

    protected void onPreExecute() {
        preview = (ImageView) childview.findViewById(R.id.game_icon);
        if (Build.VERSION.SDK_INT >= Build.VERSION_CODES.GINGERBREAD) {
            StrictMode.ThreadPolicy policy = new StrictMode.ThreadPolicy.Builder()
                    .permitAll().build();
            StrictMode.setThreadPolicy(policy);
        }
    }

    public GameInfo getGameInfo(File game) {
        if (game.getName().toLowerCase().endsWith(".elf")) { return new GameInfo(0,game.getName(),null,null);}
        TheGameDB db = new TheGameDB(mContext);
        String Serial = getSerial(game);
        GameInfo TheGame;
        if (Serial != null){
            TheGame = db.getGame(Serial, game.getName());
        } else {
            // if it has no serial it shouldn't even be a valid PS2 game image.
            // that should have been handled much earlier
            return null;
        }

        int ID = TheGame.getID();
        Bitmap frontcover = null;
        Bitmap backcover = null;
            if (ID != 0) {
                frontcover = getImage(String.valueOf(ID)+"-1");
                backcover = getImage(String.valueOf(ID)+"-2");
            } else if (TheGame.getName() != null){
                frontcover = getImage(TheGame.getName()+"-1");
                backcover = getImage(TheGame.getName()+"-2");
            }
        if (frontcover != null) {
            TheGame.setFrontCover(frontcover);
        }
        if (backcover != null) {
            TheGame.setBackCover(backcover);
        }
        if (backcover == null || frontcover == null){
                try {
                    if (isNetworkAvailable(mContext)){
                        GamesDbAPI GDB = new GamesDbAPI();
                        TheGame = GDB.getCover(TheGame, mContext, preview);
                            if (TheGame.getID() != 0) {
                                if (TheGame.getFrontCover() != null)
                                    saveImage(String.valueOf(ID) +"-1", TheGame.getFrontCover());

                                if (TheGame.getBackCover() != null)
                                    saveImage(String.valueOf(ID) +"-2", TheGame.getBackCover());
                            } else if (TheGame.getName() != null){
                                if (TheGame.getFrontCover() != null)
                                    saveImage(TheGame.getName() +"-1", TheGame.getFrontCover());

                                if (TheGame.getBackCover() != null)
                                    saveImage(TheGame.getName() +"-2", TheGame.getBackCover());
                            }
                    }
                } catch (Exception e) {
                    e.printStackTrace();
                }
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



    public static boolean isNetworkAvailable(Context mContext) {
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

    public static String getSerial(File game) {
        String serial = NativeInterop.GetDiskId(game.getAbsoluteFile().toString());
        if (serial != null && !serial.equals("")) {
            Log.d("Play!", game.getName() + " [" + serial + "]");
        }
        return serial;
    }


    private void saveImage(String key, Bitmap image){
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

    public getGameDetails(File game, Context mContext) {
        this.game = game;
        this.mContext = mContext;

    }

    public void setViewParent(View childview) {
        this.childview = childview;
    }

    @Override
    protected GameInfo doInBackground(String... params) {
        GameInfo = getGameInfo(game);
        return GameInfo;
    }

    protected void onPostExecute(GameInfo gameData) {
            if (gameData.getFrontCover() != null){
                preview.setImageBitmap(gameData.getFrontCover());
                preview.setScaleType(ImageView.ScaleType.CENTER_INSIDE);
                ((TextView) childview.findViewById(R.id.game_text)).setVisibility(View.GONE);
            } else {
                ((TextView) childview.findViewById(R.id.game_text)).setText(gameData.getName());
            }
    }
}