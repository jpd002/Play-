package com.virtualapplications.play;

public class BootablesInterop
{
	static
	{
		System.loadLibrary("Play");
	}

	public static native void scanBootables(String[] rootDirectories);
	public static native void fullScanBootables(String[] rootDirectories);
	public static native Bootable[] getBootables(int sortingMethod);
	public static native void setLastBootedTime(String bootablePath, long lastBootedTime);
	public static native void UnregisterBootable(String bootablePath);
	public static native void PurgeInexistingFiles();
}
