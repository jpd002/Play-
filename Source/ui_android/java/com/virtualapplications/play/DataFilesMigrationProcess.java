package com.virtualapplications.play;

import android.content.Context;
import android.net.Uri;
import android.util.Log;

import java.io.File;
import java.io.IOException;
import java.io.InputStream;
import java.io.OutputStream;
import java.util.Arrays;
import java.util.HashSet;

import androidx.documentfile.provider.DocumentFile;

class DataFilesMigrationProcess
{
	//This class handles migration of data from the old "Play Data Files" folder to the data folder inside the app
	//(ie.: Android 11 migration)

	//Some notes/questions for the migration process:
	//- Should we migrate the bootables.db file?
	//- Should we overwrite files systematically?
	//  No, we can assume that files in the Data Files are always older

	final Context _context;
	final String _configXmlFileName = "config.xml";
	final String _bootablesDbFileName = "bootables.db";
	final HashSet<String> _excludedFileNames = new HashSet<>(Arrays.asList(_configXmlFileName, _bootablesDbFileName));

	public DataFilesMigrationProcess(Context context)
	{
		_context = context;
	}

	public void Start(Uri srcFolderUri) throws Exception
	{
		CheckIsDataFilesFolder(srcFolderUri);
		File dataFilesFolder = new File(_context.getFilesDir().getAbsolutePath() + "/Play Data Files");
		if(!dataFilesFolder.exists())
		{
			throw new Exception("Play Data Files doesn't exist.");
		}
		DocumentFile dstFolderDoc = DocumentFile.fromFile(dataFilesFolder);
		DocumentFile srcFolderDoc = DocumentFile.fromTreeUri(_context, srcFolderUri);
		CopyTree(dstFolderDoc, srcFolderDoc);
	}

	void CheckIsDataFilesFolder(Uri srcFolderUri) throws Exception
	{
		DocumentFile folderDoc = DocumentFile.fromTreeUri(_context, srcFolderUri);
		DocumentFile configFile = folderDoc.findFile(_configXmlFileName);
		DocumentFile bootablesFile = folderDoc.findFile(_bootablesDbFileName);
		if((configFile == null) && (bootablesFile == null))
		{
			throw new Exception("Selected folder doesn't seem to be a Data Files folder.");
		}
	}

	void CopyFile(DocumentFile dstFileDoc, DocumentFile srcFileDoc) throws IOException
	{
		InputStream srcStream = _context.getContentResolver().openInputStream(srcFileDoc.getUri());
		OutputStream dstStream = _context.getContentResolver().openOutputStream(dstFileDoc.getUri());
		try
		{
			int bufferSize = 0x10000;
			byte[] buffer = new byte[bufferSize];
			while(true)
			{
				int amountRead = srcStream.read(buffer);
				if(amountRead == -1) break;
				dstStream.write(buffer, 0, amountRead);
			}
		}
		finally
		{
			srcStream.close();
			dstStream.close();
		}
	}

	void CopyTree(DocumentFile dstFolderDoc, DocumentFile srcFolderDoc)
	{
		for(DocumentFile srcFileDoc : srcFolderDoc.listFiles())
		{
			if(srcFileDoc.isDirectory())
			{
				Log.w(Constants.TAG, String.format("Creating directory '%s'...", srcFileDoc.getName()));
				DocumentFile dstFileDoc = dstFolderDoc.createDirectory(srcFileDoc.getName());
				CopyTree(dstFileDoc, srcFileDoc);
			}
			else
			{
				Log.w(Constants.TAG, String.format("Copying file '%s'...", srcFileDoc.getName()));
				try
				{
					if(dstFolderDoc.findFile(srcFileDoc.getName()) != null)
					{
						Log.w(Constants.TAG, "Skipping because it already exists.");
						continue;
					}
					if(_excludedFileNames.contains(srcFileDoc.getName()))
					{
						Log.w(Constants.TAG, "Skipping because it is excluded.");
						continue;
					}
					DocumentFile dstFileDoc = dstFolderDoc.createFile("", srcFileDoc.getName());
					CopyFile(dstFileDoc, srcFileDoc);
				}
				catch(Exception ex)
				{
					Log.e(Constants.TAG, "Failed to copy file.");
				}
			}
		}
	}
}
