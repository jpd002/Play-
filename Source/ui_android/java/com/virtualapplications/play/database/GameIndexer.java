package com.virtualapplications.play.database;

import android.content.ContentResolver;
import android.content.ContentValues;
import android.content.Context;
import android.database.Cursor;
import android.os.Environment;
import android.util.Log;

import com.virtualapplications.play.GameInfoStruct;
import com.virtualapplications.play.NativeInterop;
import com.virtualapplications.play.R;
import com.virtualapplications.play.VirtualMachineManager;

import org.apache.commons.lang3.StringUtils;

import java.io.File;
import java.io.InputStream;
import java.util.ArrayList;
import java.util.HashSet;
import java.util.List;

/**
 * Based On https://github.com/LithidSoftware/android_Findex/blob/master/src/com/lithidsw/findex/utils/FileWalker.java
 */
public class GameIndexer
{
	private Context mContext;
	String[] mediaTypes;
	private IndexingDB db;
	private List<ContentValues> toIndex;

	public GameIndexer(Context context)
	{
		this.mContext = context;
		mediaTypes = mContext.getResources().getStringArray(R.array.disks);
	}

	public static HashSet<String> getExternalMounts()
	{
		final HashSet<String> out = new HashSet<String>();
		String reg = "(?i).*vold.*(vfat|ntfs|exfat|fat32|ext3|ext4|fuse|sdfat).*rw.*";
		StringBuilder s = new StringBuilder();
		try
		{
			final java.lang.Process process = new ProcessBuilder().command("mount")
					.redirectErrorStream(true).start();
			process.waitFor();
			InputStream is = process.getInputStream();
			byte[] buffer = new byte[1024];
			while (is.read(buffer) != -1)
			{
				s.append(new String(buffer));
			}
			is.close();

			String[] lines = s.toString().split("\n");
			for (String line : lines)
			{
				if (StringUtils.containsIgnoreCase(line, "secure"))
					continue;
				if (StringUtils.containsIgnoreCase(line, "asec"))
					continue;
				if (line.matches(reg))
				{
					String[] parts = line.split(" ");
					for (String part : parts)
					{
						if (part.startsWith("/"))
							if (!StringUtils.containsIgnoreCase(part, "vold"))
								out.add(part);
					}
				}
			}
		}
		catch (final Exception e)
		{
			// SDCard Exception
		}
		return out;
	}

	public void startupScan()
	{
		List<String> paths = getPaths();
		if(paths.isEmpty())
		{
			fullScan();
		}
		else
		{
			toIndex = new ArrayList<>();
			for(String path : paths)
			{
				File file = new File(path);
				walk(file, file.getAbsolutePath(), path, 0);
			}
			insertAll();
		}
	}

	public void fullScan()
	{
		toIndex = new ArrayList<>();
		HashSet<String> extStorage = getExternalMounts();
		extStorage.add(Environment.getExternalStorageDirectory().getAbsolutePath());
		if(extStorage != null && !extStorage.isEmpty())
		{
			for(String anExtStorage : extStorage)
			{
				String sdCardPath = anExtStorage.replace("mnt/media_rw", "storage");
				File file = new File(sdCardPath);
				walk(file, file.getAbsolutePath(), sdCardPath, 9999);
			}
			insertAll();
		}
	}

	public void walk(File root, String folderPath, String storage, int depth)
	{
		File[] list = root.listFiles();
		if(list != null && !isNoMedia(list))
		{
			for(File f : list)
			{
				if(f.isDirectory() && !f.isHidden() && !isNoMediaFolder(f, storage))
				{
					if(depth <= 0) continue;
					depth--;
					Log.e("INDEX", "PATH: " + f.getAbsolutePath());
					walk(f, f.getAbsolutePath(), storage, depth);
					depth++;
				}
				else
				{
					if(!f.isHidden() && !f.isDirectory() && isCorrectExtension(f))
					{
						if(!isIndexed(folderPath, f.getName()))
						{
							String name = f.getName();
							String serial = null;
							if(!VirtualMachineManager.IsLoadableExecutableFileName(f.getPath()))
							{
								serial = getSerial(f);
								if(serial == null) continue;
							}

							ContentValues values = new ContentValues();
							values.put(IndexingDB.KEY_DISKNAME, name);
							values.put(IndexingDB.KEY_PATH, folderPath);
							values.put(IndexingDB.KEY_SERIAL, serial);
							values.put(IndexingDB.KEY_SIZE, f.length());
							values.put(IndexingDB.KEY_LAST_PLAYED, 0);
							if(serial != null)
							{
								ContentResolver cr = mContext.getContentResolver();
								Cursor c = cr.query(SqliteHelper.Games.GAMES_URI, new String[]{SqliteHelper.Games.KEY_GAMEID}, SqliteHelper.Games.KEY_SERIAL + "=?", new String[]{serial}, null);
								if(c != null)
								{
									if(c.moveToFirst())
									{
										values.put(IndexingDB.KEY_GAMEID, c.getString(c.getColumnIndex(SqliteHelper.Games.KEY_GAMEID)));
									}
									c.close();
								}
							}
							toIndex.add(values);

							//Log.i("INDEX", "Name: " + name + " PATH: " + folderPath + " Serial: " + serial + " Size " + f.length());
						}
					}
				}
			}
		}
	}

	private boolean isCorrectExtension(File f)
	{
		for(final String type : mediaTypes)
		{
			if(!(f.getParentFile().getName().startsWith(".") || f.getName().startsWith(".")))
			{
				if(StringUtils.endsWithIgnoreCase(f.getName(), "." + type))
				{
					return true;
				}
			}
		}
		return false;
	}

	public String getSerial(File game)
	{
		String serial = NativeInterop.getDiskId(game.getPath());
		//Log.d("Play!", game.getName() + " [" + serial + "]");
		return serial;
	}

	private boolean isIndexed(String absolutePath, String name)
	{
		if(db == null)
		{
			if((db = new IndexingDB(mContext)) == null)
			{
				Log.e("PLAY!", "Failed To Initiate IndexDB");
				return true;
			}
		}
		String selection = IndexingDB.KEY_DISKNAME + "=? AND " + IndexingDB.KEY_PATH + "=?";
		String[] selectionArgs = {name, absolutePath};
		return db.isIndexed(selection, selectionArgs);
	}

	private void insertAll()
	{
		if(toIndex != null && toIndex.size() > 0)
		{
			if(db == null)
			{
				if((db = new IndexingDB(mContext)) == null)
				{
					Log.e("PLAY!", "Failed To Initiate IndexDB");
				}
			}
			db.insertAll(toIndex);
			toIndex = null;
		}
		if(db != null)
		{
			db.close();
			db = null;
		}
	}

	private List<ContentValues> getIndexCVList(int sortMethod)
	{
		if(db == null)
		{
			if((db = new IndexingDB(mContext)) == null)
			{
				Log.e("PLAY!", "Failed To Initiate IndexDB");
			}
		}
		List<ContentValues> CV = db.getIndexList(sortMethod);
		db.close();
		db = null;
		return CV;
	}

	public List<GameInfoStruct> getIndexGISList(int sortMethod)
	{
		if(db == null)
		{
			if((db = new IndexingDB(mContext)) == null)
			{
				Log.e("PLAY!", "Failed To Initiate IndexDB");
			}
		}
		List<GameInfoStruct> GIS = db.getIndexGISList(sortMethod);
		db.close();
		db = null;
		return GIS;
	}

	private List<String> getPaths()
	{
		if(db == null)
		{
			if((db = new IndexingDB(mContext)) == null)
			{
				Log.e("PLAY!", "Failed To Initiate IndexDB");
			}
		}
		List<String> paths = db.getPaths();
		db.close();
		db = null;
		return paths;
	}

	private boolean isNoMedia(File[] files)
	{
		for(File file : files)
		{
			if(file.getName().equalsIgnoreCase(".nomedia"))
			{
				return true;
			}
		}

		return false;
	}

	public static boolean isNoMediaFolder(File file, String storage)
	{
		return file.getAbsolutePath().contains(storage + "/Android") ||
				file.getAbsolutePath().contains(storage + "/LOST.DIR") ||
				file.getAbsolutePath().contains(storage + "/data");
	}
}
