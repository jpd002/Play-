package com.virtualapplications.play;

import android.content.Context;
import android.content.Intent;

import java.io.File;

public class VirtualMachineManager
{
	public static void launchDisk(Context mContext, File game) throws Exception 
	{
		if(IsLoadableExecutableFileName(game.getPath()))
		{
			NativeInterop.loadElf(game.getPath());
		}
		else
		{
			NativeInterop.bootDiskImage(game.getPath());
		}
		Intent intent = new Intent(mContext, EmulatorActivity.class);
		mContext.startActivity(intent);
	}
	
	public static boolean IsLoadableExecutableFileName(String fileName)
	{
		return fileName.toLowerCase().endsWith(".elf");
	}
}
