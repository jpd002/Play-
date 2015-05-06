package com.virtualapplications.play;

import java.io.*;
import java.util.*;

class FileListItem implements Comparable<FileListItem>
{
	public FileListItem(String path)
	{
		_path = path;
		_pathFile = new File(path);
	}
	
	String getPath()
	{
		return _path;
	}
	
	boolean isParentDirectory()
	{
		return _path.equals("..");
	}
	
	boolean isDirectory()
	{
		return _pathFile.isDirectory();
	}
	
	@Override
	public int compareTo(FileListItem rhs)
	{
		int result = Boolean.compare(rhs.isParentDirectory(), isParentDirectory());
		if(result != 0) return result;
		
		result = Boolean.compare(rhs._pathFile.isDirectory(), _pathFile.isDirectory());
		if(result != 0) return result;
		
		result = _path.toLowerCase().compareTo(rhs._path.toLowerCase());
		if(result != 0) return result;

		return result;
	}
	
	@Override
	public String toString()
	{
		if(isParentDirectory())
		{
			return "[Parent Directory]";
		}
		if(_pathFile.isDirectory())
		{
			return String.format("[%s]", _pathFile.getName());
		}
		return _pathFile.getName();
	}
	
	private String _path;
	private File _pathFile;
};
