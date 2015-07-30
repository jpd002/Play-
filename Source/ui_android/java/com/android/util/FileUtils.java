package com.android.util;

import java.io.File;
import java.io.FileInputStream;
import java.io.FileOutputStream;
import java.io.FilenameFilter;
import java.io.IOException;
import java.io.ObjectInputStream;
import java.io.ObjectOutputStream;
import java.nio.IntBuffer;
import java.text.SimpleDateFormat;
import java.util.Collection;
import java.util.Date;
import java.util.List;
import java.util.Vector;
import java.util.zip.GZIPInputStream;
import java.util.zip.GZIPOutputStream;

public class FileUtils {

	public void saveArray(String filename, List<String> output_field) {
		try {
			FileOutputStream fos = new FileOutputStream(filename);
			GZIPOutputStream gzos = new GZIPOutputStream(fos);
			ObjectOutputStream out = new ObjectOutputStream(gzos);
			out.writeObject(output_field);
			out.flush();
			out.close();
		} catch (IOException e) {
			e.getStackTrace();
		}
	}

	@SuppressWarnings("unchecked")
	public List<String> loadArray(String filename) {
		try {
			FileInputStream fis = new FileInputStream(filename);
			GZIPInputStream gzis = new GZIPInputStream(fis);
			ObjectInputStream in = new ObjectInputStream(gzis);
			List<String> read_field = (List<String>) in.readObject();
			in.close();
			return read_field;
		} catch (Exception e) {
			e.getStackTrace();
		}
		return null;
	}

	public Collection<File> listFiles(File directory, FilenameFilter[] filter,
			int recurse) {

		Vector<File> files = new Vector<File>();

		File[] entries = directory.listFiles();

		if (entries != null) {
			for (File entry : entries) {
				for (FilenameFilter filefilter : filter) {
					if (filter == null
							|| filefilter.accept(directory, entry.getName())) {
						files.add(entry);
					}
				}
				if ((recurse <= -1) || (recurse > 0 && entry.isDirectory())) {
					recurse--;
					files.addAll(listFiles(entry, filter, recurse));
					recurse++;
				}
			}
		}
		return files;
	}
}