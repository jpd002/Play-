package com.virtualapplications.play;

public class BootablesInterop
{
	static
	{
		System.loadLibrary("Play");
	}

	// changes to SORT_* order should also be reflected in
	// NavigationDrawerFragment.onActivityCreated adapter string list 
	public static final int SORT_RECENT = 0;
	public static final int SORT_NONE = 1;
	public static final int SORT_HOMEBREW = 2;

	public static native void scanBootables(String[] rootDirectories);
	public static native void fullScanBootables(String[] rootDirectories);
	public static native Bootable[] getBootables(int sortingMethod);
	public static native boolean tryRegisterBootable(String bootablePath);
	public static native void setLastBootedTime(String bootablePath, long lastBootedTime);
	public static native boolean IsBootableExecutablePath(String bootablePath);
	public static native boolean DoesBootableExist(String bootablePath);
	public static native void UnregisterBootable(String bootablePath);
	public static native void fetchGameTitles();
	public static native void PurgeInexistingFiles();
}
