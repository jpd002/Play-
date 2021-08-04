package com.virtualapplications.play;

import android.content.Context;
import android.content.Intent;

public class VirtualMachineManager
{
	public static void launchDisk(Context mContext, String bootablePath) throws Exception
	{
		if(BootablesInterop.IsBootableExecutablePath(bootablePath))
		{
			NativeInterop.loadElf(bootablePath);
		}
		else
		{
			NativeInterop.bootDiskImage(bootablePath);
		}
		Intent intent = new Intent(mContext, EmulatorActivity.class);
		mContext.startActivity(intent);
	}
}
