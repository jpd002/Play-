package com.virtualapplications.play;

import android.view.*;

public class NativeInterop
{
	 static 
	 {
		 System.loadLibrary("Play");
	 }

	 public static native void setFilesDirPath(String dirPath);
	 public static native void createVirtualMachine();
	 public static native void start(Surface surface, String selectedFilePath);
}
