package com.virtualapplications.play;

import android.app.*;
import android.content.*;
import android.os.*;
import android.util.*;
import android.view.*;
import android.widget.*;
import java.util.*;

public class EmulatorActivity extends Activity 
{
	private SurfaceView _renderView;
	private TextView _statsTextView;
	private Timer _statsTimer = new Timer();
	private Handler _statsTimerHandler;
	
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
		
		final SharedPreferences _preferences = getSharedPreferences("prefs", MODE_PRIVATE);
		if (_preferences.getBoolean("fpsValue", false))
		{
			_statsTextView = (TextView)findViewById(R.id.emulator_stats);
			setupStatsTimer();
		}
	}
	
	@Override
	public void onPause()
	{
		super.onPause();
		NativeInterop.pauseVirtualMachine();
	}
	
	@Override
	public void onDestroy()
	{
		_statsTimer.cancel();
		super.onDestroy();
	}
	
	private void setupStatsTimer()
	{
		_statsTimerHandler = 
			new Handler()
			{
				@Override
				public void handleMessage(Message message)
				{
					int frames = StatsManager.getFrames();
					int drawCalls = StatsManager.getDrawCalls();
					int dcpf = (frames != 0) ? (drawCalls / frames) : 0;
					_statsTextView.setText(String.format("%d f/s, %d dc/f", frames, dcpf));
					StatsManager.clearStats();
				}
			};
		
		_statsTimer.schedule(
			new TimerTask() 
			{
				@Override
				public void run()
				{
					_statsTimerHandler.obtainMessage().sendToTarget();
				}
			},
			0,
			1000);
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
