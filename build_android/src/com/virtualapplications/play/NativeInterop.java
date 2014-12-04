package com.virtualapplications.play;

public class NativeInterop
{
	 static 
	 {
		 System.loadLibrary("Play");
	 }

	 public static native void start();
}
