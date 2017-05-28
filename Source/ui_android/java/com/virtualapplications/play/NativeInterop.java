package com.virtualapplications.play;

import android.content.res.AssetManager;
import android.view.Surface;

public class NativeInterop
{
	static 
	{
		System.loadLibrary("Play");
	}

	public static native void setFilesDirPath(String dirPath);
	public static native void setAssetManager(AssetManager assetManager);
	
	public static native void createVirtualMachine();
	public static native boolean isVirtualMachineCreated();
	public static native boolean isVirtualMachineRunning();
	public static native void resumeVirtualMachine();
	public static native void pauseVirtualMachine();
	 
	public static native void loadState(int slot);
	public static native void saveState(int slot);
	
	public static native void loadElf(String selectedFilePath);
	public static native void bootDiskImage(String selectedFilePath);
	
	public static native void setupGsHandler(Surface surface);
	
	public static native String getDiskId(String diskImagePath);
}
