package com.virtualapplications.play;

import android.app.Activity;
import android.os.Bundle;
import android.util.Log;
import android.view.*;
import android.view.View.*;
import android.widget.*;
import java.io.*;

public class MainActivity extends Activity 
{
	private SurfaceView _renderView;

	private static String TAG = "Play!";
	
	private class SurfaceCallback implements SurfaceHolder.Callback
	{
		@Override public void surfaceChanged(SurfaceHolder holder, int format, int width, int height)
		{
			Log.w(TAG, String.format("surfaceChanged -> format: %d, width: %d, height: %d", format, width, height));
			Surface surface = holder.getSurface();
			NativeInterop.start(surface);
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
		
		((Button)findViewById(R.id.startTests)).setOnClickListener(
			new OnClickListener() 
			{
				public void onClick(View view) 
				{
					setContentView(_renderView);
					SurfaceHolder holder = _renderView.getHolder();
					holder.addCallback(new SurfaceCallback());
				}
			}
		);
	}
}
