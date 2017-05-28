package com.virtualapplications.play;

public class InputManager
{
	static 
	{
		System.loadLibrary("Play");
	}

	public static native void setButtonState(int button, boolean pressed);
	public static native void setAxisState(int button, float value);
	
	public static native String getVirtualPadItems(float screenWidth, float screenHeight);
}
