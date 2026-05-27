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

class DataFilesImpExpProcess
{
	public enum ImpExpAction
	{
		Import,
		Export
	};

	final Context _context;
	final String _configXmlFileName = "config.xml";
	final String _bootablesDbFileName = "bootables.db";
	final HashSet<String> _excludedFileNames = new HashSet<>(Arrays.asList(_configXmlFileName, _bootablesDbFileName));

	public DataFilesImpExpProcess(Context context)
	{
		_context = context;
	}

	public void Start(ImpExpAction action, Uri targetFolderUri) throws Exception
	{
		File dataFilesFolder = new File(_context.getFilesDir().getAbsolutePath() + "/Play Data Files");
		if(!dataFilesFolder.exists())
		{
			throw new Exception("Play Data Files doesn't exist.");
		}
		switch(action)
		{
		case Import:
		{
			//Import to app's Data Files folder
			DocumentFile dstFolderDoc = DocumentFile.fromFile(dataFilesFolder);
			DocumentFile srcFolderDoc = DocumentFile.fromTreeUri(_context, targetFolderUri);
			CopyTree(dstFolderDoc, srcFolderDoc);
		}
		break;
		case Export:
		{
			//Export to folder on device
			DocumentFile dstFolderDoc = DocumentFile.fromTreeUri(_context, targetFolderUri);
			DocumentFile srcFolderDoc = DocumentFile.fromFile(dataFilesFolder);
			CopyTree(dstFolderDoc, srcFolderDoc);
		}
		break;
		}
	}

	void CopyFile(DocumentFile dstFileDoc, DocumentFile srcFileDoc) throws IOException
	{
		InputStream srcStream = _context.getContentResolver().openInputStream(srcFileDoc.getUri());
		OutputStream dstStream = _context.getContentResolver().openOutputStream(dstFileDoc.getUri(), "wt");
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
				Log.w(Constants.TAG, String.format("Copying directory '%s'...", srcFileDoc.getName()));
				DocumentFile targetFolderDoc = dstFolderDoc.findFile(srcFileDoc.getName());
				if(targetFolderDoc == null)
				{
					Log.w(Constants.TAG, String.format("Creating directory '%s'...", srcFileDoc.getName()));
					targetFolderDoc = dstFolderDoc.createDirectory(srcFileDoc.getName());
				}
				CopyTree(targetFolderDoc, srcFileDoc);
			}
			else
			{
				Log.w(Constants.TAG, String.format("Copying file '%s'...", srcFileDoc.getName()));
				try
				{
					if(_excludedFileNames.contains(srcFileDoc.getName()))
					{
						Log.w(Constants.TAG, "Skipping because it is excluded.");
						continue;
					}
					DocumentFile targetFileDoc = dstFolderDoc.findFile(srcFileDoc.getName());
					if(targetFileDoc == null)
					{
						Log.w(Constants.TAG, String.format("Creating file '%s'...", srcFileDoc.getName()));
						targetFileDoc = dstFolderDoc.createFile("", srcFileDoc.getName());
					}
					CopyFile(targetFileDoc, srcFileDoc);
				}
				catch(Exception ex)
				{
					Log.e(Constants.TAG, "Failed to copy file.");
				}
			}
		}
	}
}
