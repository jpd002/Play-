package com.virtualapplications.play.database;

import android.content.ContentValues;
import android.content.Context;
import android.os.Environment;
import android.util.Log;

import com.virtualapplications.play.GameInfoStruct;
import com.virtualapplications.play.NativeInterop;
import com.virtualapplications.play.R;

import org.apache.commons.lang3.StringUtils;

import java.io.File;
import java.util.HashSet;
import java.util.List;

import static com.virtualapplications.play.MainActivity.IsLoadableExecutableFileName;
import static com.virtualapplications.play.MainActivity.getExternalMounts;

/**
 * Based On https://github.com/LithidSoftware/android_Findex/blob/master/src/com/lithidsw/findex/utils/FileWalker.java
 */
public class GameIndexer {

    private Context mContext;
    String[] mediaTypes;
    private IndexingDB db;


    public GameIndexer(Context context){
        this.mContext = context;
        mediaTypes = mContext.getResources().getStringArray(R.array.disks);
        db = new IndexingDB(mContext);
    }

    public void startupindexingscan() {
        long startTime = System.currentTimeMillis();
        List<String> paths = getpath();
        if (paths.isEmpty()) {
            HashSet<String> extStorage = getExternalMounts();
            extStorage.add(Environment.getExternalStorageDirectory().getAbsolutePath());
            if (extStorage != null && !extStorage.isEmpty()) {
                for (String anExtStorage : extStorage) {
                    String sdCardPath = anExtStorage.replace("mnt/media_rw", "storage");
                    File file = new File(sdCardPath);
                    walk(file, file.getAbsolutePath(), sdCardPath, 9999);
                }
            }
            Log.i("INDEX", " 1st Runs");
        } else {
            for (String path : paths) {
                File file = new File(path);
                walk(file, file.getAbsolutePath(), path, 0);
            }
            Log.i("INDEX", " 2nd Runs");
        }
        long endTime   = System.currentTimeMillis();
        long totalTime = endTime - startTime;
        Log.i("INDEX", "MY Time " + totalTime);
    }

    public void fullindexingscan() {
            HashSet<String> extStorage = getExternalMounts();
            extStorage.add(Environment.getExternalStorageDirectory().getAbsolutePath());
            if (extStorage != null && !extStorage.isEmpty()) {
                for (String anExtStorage : extStorage) {
                    String sdCardPath = anExtStorage.replace("mnt/media_rw", "storage");
                    File file = new File(sdCardPath);
                    walk(file, file.getAbsolutePath(), sdCardPath, 9999);
                }
            }
            Log.i("INDEX", " 1st Runs");

    }

    public void walk(File root, String folderPath, String storage, int depth) {
        File[] list = root.listFiles();
        if (list != null && !isNoMedia(list)) {
            for (File f : list) {
                if (f.isDirectory() && !f.isHidden() && !isNoMediaFolder(f, storage)) {
                    if (depth <=0) continue;
                    depth--;
                    Log.e("INDEX", "PATH: " + f.getAbsolutePath());
                    walk(f, f.getAbsolutePath(), storage, depth);
                    depth++;
                } else {
                    if (!f.isHidden() && !f.isDirectory() && isCorrectExtension(f)) {
                        if (!isIndexed(folderPath, f.getName())) {
                            String name = f.getName();
                            String serial = null;
                            if (!IsLoadableExecutableFileName(f.getPath())) {
                                serial = getSerial(f);
                            }

                            ContentValues values = new ContentValues();
                            values.put(IndexingDB.KEY_DISKNAME, name);
                            values.put(IndexingDB.KEY_PATH, folderPath);
                            values.put(IndexingDB.KEY_SERIAL, serial);
                            values.put(IndexingDB.KEY_SIZE, f.length());
                            values.put(IndexingDB.KEY_LAST_PLAYED, f.lastModified());
                            insert(values);
                            //Log.i("INDEX", "Name: " + name + " PATH: " + folderPath + " Serial: " + serial + " Size " + f.length());
                        }
                    }
                }
            }
        }
    }

    private boolean isCorrectExtension(File f) {
        for (final String type : mediaTypes) {
            if (!(f.getParentFile().getName().startsWith(".") || f.getName().startsWith("."))) {
                if (StringUtils.endsWithIgnoreCase(f.getName(), "." + type)) {
                    String serial = getSerial(f);
                    return IsLoadableExecutableFileName(f.getPath()) ||
                            (serial != null && !serial.equals(""));
                }
            }
        }
        return false;
    }

    public String getSerial(File game) {
        String serial = NativeInterop.getDiskId(game.getPath());
        Log.d("Play!", game.getName() + " [" + serial + "]");
        return serial;
    }

    private boolean isIndexed(String absolutePath, String name) {
        if (db == null){
            if ((db = new IndexingDB(mContext)) == null){
                Log.e("PLAY!", "Failed To Initiate IndexDB");
                return true;
            }
        }
        String selection = IndexingDB.KEY_DISKNAME + "=? AND " + IndexingDB.KEY_PATH + "=?";
        String[] selectionArgs = {name, absolutePath};
        return db.isIndexed(selection, selectionArgs);

    }

    private void insert(ContentValues values) {
        if (db == null){
            if ((db = new IndexingDB(mContext)) == null){
                Log.e("PLAY!", "Failed To Initiate IndexDB");
            }
        }
        db.insert(values);
    }

    private List<ContentValues> getindex() {
        if (db == null){
            if ((db = new IndexingDB(mContext)) == null){
                Log.e("PLAY!", "Failed To Initiate IndexDB");
            }
        }
        return db.getAllIndexList();
    }

    public List<GameInfoStruct> getindexGameInfoStruct(boolean homebrew) {
        if (db == null){
            if ((db = new IndexingDB(mContext)) == null){
                Log.e("PLAY!", "Failed To Initiate IndexDB");
            }
        }
        return db.getAllIndexGameInfoStruct(homebrew);
    }

    private List<String> getpath() {
        if (db == null){
            if ((db = new IndexingDB(mContext)) == null){
                Log.e("PLAY!", "Failed To Initiate IndexDB");
            }
        }
        return db.getPaths();
    }

    private boolean isNoMedia(File[] files) {
        for (File file : files) {
            if (file.getName().equalsIgnoreCase(".nomedia")) {
                return true;
            }
        }

        return false;
    }

    public static boolean isNoMediaFolder(File file, String storage) {
        return file.getAbsolutePath().contains(storage + "/Android") ||
                file.getAbsolutePath().contains(storage + "/LOST.DIR") ||
                file.getAbsolutePath().contains(storage + "/data");

    }
}
