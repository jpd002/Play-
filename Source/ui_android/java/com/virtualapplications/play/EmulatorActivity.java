package com.virtualapplications.play;

import android.app.*;
import android.content.*;
import android.content.res.Configuration;
import android.content.res.TypedArray;
import android.os.*;
import android.graphics.Color;
import android.support.v4.widget.DrawerLayout;
import android.util.*;
import android.view.*;
import android.widget.*;
import android.widget.Toast;

import java.util.*;

public class EmulatorActivity extends Activity
{
	private static final String PREFERENCE_UI_SHOWFPS = "ui.showfps";
	private static final String PREFERENCE_UI_SHOWVIRTUALPAD = "ui.showvirtualpad";
	private SurfaceView _renderView;
	private Timer _statsTimer = new Timer();
	private Handler _statsTimerHandler;
	private TextView _fpsTextView;
	private TextView _profileTextView;
	private boolean _surfaceCreated = false;
	private boolean _activityRunning = false;
	private DrawerLayout _drawerLayout;
	protected EmulatorDrawerFragment _drawerFragment;

	public static void RegisterPreferences()
	{
		SettingsManager.registerPreferenceBoolean(PREFERENCE_UI_SHOWFPS, false);
		SettingsManager.registerPreferenceBoolean(PREFERENCE_UI_SHOWVIRTUALPAD, true);
	}

	@Override
	protected void onCreate(Bundle savedInstanceState)
	{
		super.onCreate(savedInstanceState);
		//Log.w(Constants.TAG, "EmulatorActivity - onCreate");

		ThemeManager.applyTheme(this);
		setContentView(R.layout.emulator);

		getWindow().addFlags(WindowManager.LayoutParams.FLAG_KEEP_SCREEN_ON);
		getWindow().getDecorView().setSystemUiVisibility(View.SYSTEM_UI_FLAG_LAYOUT_STABLE |
				View.SYSTEM_UI_FLAG_LAYOUT_HIDE_NAVIGATION |
				View.SYSTEM_UI_FLAG_LAYOUT_FULLSCREEN |
				View.SYSTEM_UI_FLAG_HIDE_NAVIGATION |
				View.SYSTEM_UI_FLAG_FULLSCREEN |
				View.SYSTEM_UI_FLAG_IMMERSIVE_STICKY);

		_drawerFragment = (EmulatorDrawerFragment)getFragmentManager().findFragmentById(R.id.emulator_drawer);
		_drawerFragment.setEventListener(
				new EmulatorDrawerFragment.EventListener()
				{
					@Override
					public void onExitSelected()
					{
						finish();
					}

					@Override
					public void onSaveStateSelected()
					{
						int messageStringId = R.string.emulator_save_state_ok;
						try
						{
							NativeInterop.saveState(0);
						}
						catch(Exception e)
						{
							messageStringId = R.string.emulator_save_state_fail;
						}
						Toast toast = Toast.makeText(
								getApplicationContext(), getString(messageStringId), Toast.LENGTH_SHORT
						);
						toast.show();
						_drawerFragment.closeDrawer();
					}

					@Override
					public void onLoadStateSelected()
					{
						int messageStringId = R.string.emulator_load_state_ok;
						try
						{
							NativeInterop.loadState(0);
						}
						catch(Exception e)
						{
							messageStringId = R.string.emulator_load_state_fail;
						}
						Toast toast = Toast.makeText(
								getApplicationContext(), getString(messageStringId), Toast.LENGTH_SHORT
						);
						toast.show();
						_drawerFragment.closeDrawer();
					}
				}
		);

		View fragmentView = findViewById(R.id.emulator_drawer);
		_drawerLayout = (DrawerLayout)findViewById(R.id.emulator_drawer_layout);
		_drawerFragment.setUp(fragmentView, _drawerLayout);
	}

	@Override
	protected void onPostCreate(Bundle savedInstanceState)
	{
		super.onPostCreate(savedInstanceState);

		_renderView = (SurfaceView)findViewById(R.id.emulator_view);
		SurfaceHolder holder = _renderView.getHolder();
		holder.addCallback(new SurfaceCallback());

		_fpsTextView = (TextView)findViewById(R.id.emulator_fps);
		_profileTextView = (TextView)findViewById(R.id.emulator_profile);

		if(!SettingsManager.getPreferenceBoolean(PREFERENCE_UI_SHOWVIRTUALPAD))
		{
			View virtualPadView = (View)findViewById(R.id.emulator_virtualpad);
			virtualPadView.setVisibility(View.GONE);
		}

		if(
				SettingsManager.getPreferenceBoolean(PREFERENCE_UI_SHOWFPS) ||
						StatsManager.isProfiling()
				)
		{
			setupStatsTimer();
		}
	}

	@Override
	public void onPause()
	{
		super.onPause();
		_activityRunning = false;
		updateVirtualMachineState();
	}

	@Override
	public void onResume()
	{
		super.onResume();
		_activityRunning = true;
		updateVirtualMachineState();
	}

	@Override
	public void onDestroy()
	{
		_statsTimer.cancel();
		super.onDestroy();
	}

	@Override
	public boolean dispatchKeyEvent(KeyEvent event)
	{
		if(_drawerFragment.isDrawerOpened())
		{
			return super.dispatchKeyEvent(event);
		}
		int action = event.getAction();
		if((action == KeyEvent.ACTION_DOWN) || (action == KeyEvent.ACTION_UP))
		{
			boolean pressed = (action == KeyEvent.ACTION_DOWN);
			switch(event.getKeyCode())
			{
			case KeyEvent.KEYCODE_DPAD_UP:
				InputManager.setButtonState(InputManagerConstants.BUTTON_UP, pressed);
				return true;
			case KeyEvent.KEYCODE_DPAD_DOWN:
				InputManager.setButtonState(InputManagerConstants.BUTTON_DOWN, pressed);
				return true;
			case KeyEvent.KEYCODE_DPAD_LEFT:
				InputManager.setButtonState(InputManagerConstants.BUTTON_LEFT, pressed);
				return true;
			case KeyEvent.KEYCODE_DPAD_RIGHT:
				InputManager.setButtonState(InputManagerConstants.BUTTON_RIGHT, pressed);
				return true;
			case KeyEvent.KEYCODE_BUTTON_A:
				InputManager.setButtonState(InputManagerConstants.BUTTON_CROSS, pressed);
				return true;
			case KeyEvent.KEYCODE_BUTTON_B:
				InputManager.setButtonState(InputManagerConstants.BUTTON_CIRCLE, pressed);
				return true;
			case KeyEvent.KEYCODE_BUTTON_X:
				InputManager.setButtonState(InputManagerConstants.BUTTON_SQUARE, pressed);
				return true;
			case KeyEvent.KEYCODE_BUTTON_Y:
				InputManager.setButtonState(InputManagerConstants.BUTTON_TRIANGLE, pressed);
				return true;
			case KeyEvent.KEYCODE_BUTTON_L1:
				InputManager.setButtonState(InputManagerConstants.BUTTON_L1, pressed);
				return true;
			case KeyEvent.KEYCODE_BUTTON_L2:
				InputManager.setButtonState(InputManagerConstants.BUTTON_L2, pressed);
				return true;
			case KeyEvent.KEYCODE_BUTTON_R1:
				InputManager.setButtonState(InputManagerConstants.BUTTON_R1, pressed);
				return true;
			case KeyEvent.KEYCODE_BUTTON_R2:
				InputManager.setButtonState(InputManagerConstants.BUTTON_R2, pressed);
				return true;
			case KeyEvent.KEYCODE_BUTTON_START:
				InputManager.setButtonState(InputManagerConstants.BUTTON_START, pressed);
				return true;
			case KeyEvent.KEYCODE_BUTTON_SELECT:
				InputManager.setButtonState(InputManagerConstants.BUTTON_SELECT, pressed);
				return true;
			}
		}
		//Log.w(Constants.TAG, String.format("Key event %d, device id %d.", event.getKeyCode(), event.getDeviceId()));
		return super.dispatchKeyEvent(event);
	}

	@Override
	public boolean dispatchGenericMotionEvent(MotionEvent event)
	{
		int action = event.getActionMasked();
		if(action == MotionEvent.ACTION_MOVE)
		{
			float lx = event.getAxisValue(MotionEvent.AXIS_X);
			float ly = event.getAxisValue(MotionEvent.AXIS_Y);
			float rx = event.getAxisValue(MotionEvent.AXIS_Z);
			float ry = event.getAxisValue(MotionEvent.AXIS_RZ);
			InputManager.setAxisState(InputManagerConstants.ANALOG_LEFT_X, lx);
			InputManager.setAxisState(InputManagerConstants.ANALOG_LEFT_Y, ly);
			InputManager.setAxisState(InputManagerConstants.ANALOG_RIGHT_X, rx);
			InputManager.setAxisState(InputManagerConstants.ANALOG_RIGHT_Y, ry);
			//Log.w(Constants.TAG, String.format("lx: %f, ly: %f, rx: %f, ry: %f", lx, ly, rx, ry));
		}
		return super.dispatchGenericMotionEvent(event);
	}

	@Override
	public void onBackPressed()
	{
		if(_drawerFragment.isDrawerOpened())
		{
			_drawerFragment.closeDrawer();
		}
		else
		{
			_drawerFragment.openDrawer();
		}
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
						_fpsTextView.setText(String.format("%d f/s, %d dc/f", frames, dcpf));
						if(StatsManager.isProfiling())
						{
							String profilingInfo = StatsManager.getProfilingInfo();
							_profileTextView.setText(profilingInfo);
						}
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

	private void updateVirtualMachineState()
	{
		//State must not be touched if surface wasn't created
		if(!_surfaceCreated) return;
		if(_activityRunning)
		{
			NativeInterop.resumeVirtualMachine();
		}
		else
		{
			NativeInterop.pauseVirtualMachine();
		}
	}

	private class SurfaceCallback implements SurfaceHolder.Callback
	{
		@Override
		public void surfaceChanged(SurfaceHolder holder, int format, int width, int height)
		{
			//Log.w(Constants.TAG, String.format("surfaceChanged -> format: %d, width: %d, height: %d", format, width, height));
			Surface surface = holder.getSurface();
			NativeInterop.setupGsHandler(surface);
			_surfaceCreated = true;
			updateVirtualMachineState();
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
