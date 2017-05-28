package com.virtualapplications.play;

public class SettingsManager
{
	static 
	{
		System.loadLibrary("Play");
	}

	public static native void save();
	
	public static native void registerPreferenceBoolean(String name, boolean defaultValue);
	public static native boolean getPreferenceBoolean(String name);
	public static native void setPreferenceBoolean(String name, boolean value);
}
