package com.virtualapplications.play.database;

import android.os.Environment;

import org.apache.commons.lang3.StringUtils;

import java.io.File;
import java.io.InputStream;
import java.util.HashSet;

import static com.virtualapplications.play.BootablesInterop.fullScanBootables;
import static com.virtualapplications.play.BootablesInterop.scanBootables;

public class GameIndexer
{

	private static HashSet<String> getExternalMounts()
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

	public static void startupScan()
	{
		HashSet<String> extStorage = getExternalMounts();
		extStorage.add(Environment.getExternalStorageDirectory().getAbsolutePath());
		scanBootables(extStorage.toArray(new String[extStorage.size()]));
	}

	public static void fullScan()
	{
		HashSet<String> extStorage = getExternalMounts();
		extStorage.add(Environment.getExternalStorageDirectory().getAbsolutePath());
		if(!extStorage.isEmpty())
		{
			fullScanBootables(extStorage.toArray(new String[extStorage.size()]));
		}
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
