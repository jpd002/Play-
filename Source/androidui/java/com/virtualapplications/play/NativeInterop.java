package com.virtualapplications.play;

public class NativeInterop
{
	 static 
	 {
		 System.loadLibrary("Play");
	 }

	 public static native void setFilesDirPath(String dirPath);
	 public static native void createVirtualMachine();
	 public static native void start();
}
