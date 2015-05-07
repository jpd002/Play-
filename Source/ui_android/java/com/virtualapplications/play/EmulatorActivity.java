package com.virtualapplications.play;

import android.app.*;
import android.os.*;
import android.util.*;
import android.view.*;

public class EmulatorActivity extends Activity 
{
	private SurfaceView _renderView;
	
	@Override 
	protected void onCreate(Bundle savedInstanceState) 
	{
		super.onCreate(savedInstanceState);
		//Log.w(Constants.TAG, "EmulatorActivity - onCreate");
		
		getWindow().addFlags(WindowManager.LayoutParams.FLAG_KEEP_SCREEN_ON);
		
		_renderView = new SurfaceView(this);
		SurfaceHolder holder = _renderView.getHolder();
		holder.addCallback(new SurfaceCallback());
		
		_renderView.setOnTouchListener(
			new View.OnTouchListener()
			{
				@Override
				public boolean onTouch(View view, MotionEvent event)
				{
					switch(event.getActionMasked())
					{
					case MotionEvent.ACTION_DOWN:
						NativeInterop.reportInput(true);
						break;
					case MotionEvent.ACTION_UP:
						NativeInterop.reportInput(false);
						break;
					}
					return true;
				}
			}
		);
		
		setContentView(_renderView);
	}
	
	@Override
	public void onPause()
	{
		super.onPause();
		NativeInterop.pauseVirtualMachine();
	}
	
	private class SurfaceCallback implements SurfaceHolder.Callback
	{
		@Override 
		public void surfaceChanged(SurfaceHolder holder, int format, int width, int height)
		{
			//Log.w(Constants.TAG, String.format("surfaceChanged -> format: %d, width: %d, height: %d", format, width, height));
			Surface surface = holder.getSurface();
			NativeInterop.setupGsHandler(surface);
			NativeInterop.resumeVirtualMachine();
		}
		
		@Override 
		public void surfaceCreated(SurfaceHolder holder)
		{
			Log.w(Constants.TAG, "surfaceCreated");
		}
		
		@Override 
		public void surfaceDestroyed(SurfaceHolder holder)
		{
			Log.w(Constants.TAG, "surfaceDestroyed");
		}
	}
}
