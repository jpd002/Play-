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
		
		setContentView(R.layout.emulator);

		getWindow().addFlags(WindowManager.LayoutParams.FLAG_KEEP_SCREEN_ON);
		getWindow().getDecorView().setSystemUiVisibility(View.SYSTEM_UI_FLAG_LAYOUT_STABLE |
				View.SYSTEM_UI_FLAG_LAYOUT_HIDE_NAVIGATION |
				View.SYSTEM_UI_FLAG_LAYOUT_FULLSCREEN |
				View.SYSTEM_UI_FLAG_HIDE_NAVIGATION |
				View.SYSTEM_UI_FLAG_FULLSCREEN |
				View.SYSTEM_UI_FLAG_IMMERSIVE_STICKY);
		
		_renderView = (SurfaceView)findViewById(R.id.emulator_view);
		SurfaceHolder holder = _renderView.getHolder();
		holder.addCallback(new SurfaceCallback());
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
