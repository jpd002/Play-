package com.virtualapplications.play;

import android.app.*;
import android.content.*;
import android.os.*;
import android.util.*;
import android.view.*;
import android.widget.*;
import android.widget.AdapterView.*;
import java.io.*;
import java.util.*;

public class MainActivity extends Activity 
{	
	private static final String PREFERENCE_CURRENT_DIRECTORY = "CurrentDirectory";
	
	private SharedPreferences _preferences;
	private FileListItem[] _fileListItems;
	private ListView _fileListView;
	private TextView _fileListViewHeader;
	
	@Override 
	protected void onCreate(Bundle savedInstanceState) 
	{
		super.onCreate(savedInstanceState);
		//Log.w(Constants.TAG, "MainActivity - onCreate");
		
		setContentView(R.layout.main);
		
		_preferences = getPreferences(MODE_PRIVATE);
		
		File filesDir = getFilesDir();
		NativeInterop.setFilesDirPath(Environment.getExternalStorageDirectory().getAbsolutePath());
	
		if(!NativeInterop.isVirtualMachineCreated())
		{
			NativeInterop.createVirtualMachine();
		}
		
		prepareFileListView();
		updateFileListView();
	}
	
	private String getCurrentDirectory()
	{
		return _preferences.getString(PREFERENCE_CURRENT_DIRECTORY, Environment.getExternalStorageDirectory().getAbsolutePath());
	}
	
	private void setCurrentDirectory(String currentDirectory)
	{
		SharedPreferences.Editor preferencesEditor = _preferences.edit();
		preferencesEditor.putString(PREFERENCE_CURRENT_DIRECTORY, currentDirectory);
		preferencesEditor.commit();
	}
	
	private static FileListItem[] getFileList(String directoryPath)
	{
		ArrayList<FileListItem> fileListItems = new ArrayList<FileListItem>();
		fileListItems.add(new FileListItem(".."));
		File directoryFile = new File(directoryPath);
		File[] fileList = directoryFile.listFiles();
		if(fileList != null)
		{
			for(File file : fileList)
			{
				if(file.isDirectory())
				{
					fileListItems.add(new FileListItem(file.getAbsolutePath()));
				}
				else if(file.getName().endsWith(".elf"))
				{
					fileListItems.add(new FileListItem(file.getAbsolutePath()));
				}
			}
		}
		Collections.sort(fileListItems);
		return fileListItems.toArray(new FileListItem[fileListItems.size()]);
	}
	
	private void prepareFileListView()
	{
		_fileListView = (ListView)findViewById(R.id.fileList);
		
		View header = (View)getLayoutInflater().inflate(R.layout.fileview_header, null);
		_fileListViewHeader = (TextView)header.findViewById(R.id.headerTextView);

		_fileListView.addHeaderView(header, null, false);
		
		_fileListView.setOnItemClickListener(
			new OnItemClickListener() 
			{
				@Override 
				public void onItemClick(AdapterView<?> parent, View view, int position, long id) 
				{
					//Remove one to compensate for header
					position = position - 1;
					
					FileListItem fileListItem = _fileListItems[position];
					if(fileListItem.isParentDirectory())
					{
						File currentDirectoryFile = new File(getCurrentDirectory());
						File parentDirectoryFile = currentDirectoryFile.getParentFile();
						if(parentDirectoryFile == null) return;
						setCurrentDirectory(currentDirectoryFile.getParentFile().getAbsolutePath());
						updateFileListView();
					}
					else if(fileListItem.isDirectory())
					{
						setCurrentDirectory(fileListItem.getPath());
						updateFileListView();
					}
					else
					{
						NativeInterop.loadElf(fileListItem.getPath());
						//TODO: Catch errors that might happen while loading files
						Intent intent = new Intent(getApplicationContext(), EmulatorActivity.class);
						startActivity(intent);
					}
				}
			}
		);
	}
	
	private void updateFileListView()
	{
		_fileListItems = getFileList(getCurrentDirectory());
		_fileListViewHeader.setText(getCurrentDirectory());
		
		ArrayAdapter<FileListItem> adapter = new ArrayAdapter<FileListItem>(this,
			android.R.layout.simple_list_item_1, android.R.id.text1, _fileListItems);
		_fileListView.setAdapter(adapter);
	}
}
