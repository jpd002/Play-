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
	private String[] _loadableFiles = getLoadableFiles();
	
	@Override 
	protected void onCreate(Bundle savedInstanceState) 
	{
		super.onCreate(savedInstanceState);
		Log.w(Constants.TAG, "MainActivity - onCreate");
		
		setContentView(R.layout.main);
		
		File filesDir = getFilesDir();
		NativeInterop.setFilesDirPath(Environment.getExternalStorageDirectory().getAbsolutePath());
	
		if(!NativeInterop.isVirtualMachineCreated())
		{
			NativeInterop.createVirtualMachine();
		}
		
		setupFileList();
	}

	private String[] getLoadableFiles()
	{
		ArrayList<String> loadableFiles = new ArrayList<String>();
		File externalStorageDir = new File(Environment.getExternalStorageDirectory().getAbsolutePath());
		externalStorageDir.listFiles();
		for(File file : externalStorageDir.listFiles())
		{
			if(file.getName().endsWith(".elf"))
			{
				loadableFiles.add(file.getAbsolutePath());
			}
		}
		return loadableFiles.toArray(new String[loadableFiles.size()]);
	}
	
	private void setupFileList()
	{
		ListView listView = (ListView)findViewById(R.id.fileList);

		ArrayAdapter<String> adapter = new ArrayAdapter<String>(this,
			android.R.layout.simple_list_item_1, android.R.id.text1, _loadableFiles);
		listView.setAdapter(adapter);
		
		listView.setOnItemClickListener(
			new OnItemClickListener() 
			{
				@Override 
				public void onItemClick(AdapterView<?> parent, View view, int position, long id) 
				{
					String selectedFilePath = _loadableFiles[position];
					NativeInterop.loadElf(selectedFilePath);
					//TODO: Catch errors that might happen while loading files
					Intent intent = new Intent(getApplicationContext(), EmulatorActivity.class);
					startActivity(intent);
				}
			}
		);
	}
}
