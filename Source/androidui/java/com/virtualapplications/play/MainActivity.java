package com.virtualapplications.play;

import android.app.Activity;
import android.os.*;
import android.util.Log;
import android.view.*;
import android.view.View.*;
import android.widget.*;
import android.widget.AdapterView.*;
import java.io.*;
import java.util.*;

public class MainActivity extends Activity 
{
	private SurfaceView _renderView;
	private String[] _loadableFiles = getLoadableFiles();
	
	private static String TAG = "Play!";
	
	private class SurfaceCallback implements SurfaceHolder.Callback
	{
		private String _selectedFilePath;
		
		public SurfaceCallback(String selectedFilePath)
		{
			_selectedFilePath = selectedFilePath;
		}
		
		@Override public void surfaceChanged(SurfaceHolder holder, int format, int width, int height)
		{
			Log.w(TAG, String.format("surfaceChanged -> format: %d, width: %d, height: %d", format, width, height));
			Surface surface = holder.getSurface();
			NativeInterop.start(surface, _selectedFilePath);
		}
		
		@Override public void surfaceCreated(SurfaceHolder holder)
		{
			Log.w(TAG, "surfaceCreated");
		}
		
		@Override public void surfaceDestroyed(SurfaceHolder holder)
		{
			Log.w(TAG, "surfaceDestroyed");
		}
	}
	
	@Override protected void onCreate(Bundle icicle) 
	{
		super.onCreate(icicle);
		setContentView(R.layout.main);
		
		_renderView = new SurfaceView(this);
		
		File filesDir = getFilesDir();
		NativeInterop.setFilesDirPath(filesDir.getAbsolutePath());
		
		NativeInterop.createVirtualMachine();
		
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
					setContentView(_renderView);
					SurfaceHolder holder = _renderView.getHolder();
					holder.addCallback(new SurfaceCallback(selectedFilePath));
				}
			}
		);
	}
}
