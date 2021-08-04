package com.virtualapplications.play;

import android.app.Activity;
import android.app.AlertDialog;
import android.content.Intent;
import android.os.Bundle;
import android.os.Environment;

import java.io.File;

public class ExternalEmulatorLauncher extends Activity
{
	@Override
	public void onCreate(Bundle savedInstanceState)
	{
		super.onCreate(savedInstanceState);
		// TODO: This method is called when the BroadcastReceiver is receiving
		// an Intent broadcast.
		NativeInterop.setFilesDirPath(Environment.getExternalStorageDirectory().getAbsolutePath());
		NativeInterop.setCacheDirPath(getApplicationContext().getCacheDir().getAbsolutePath());
		NativeInterop.setAssetManager(getAssets());

		EmulatorActivity.RegisterPreferences();

		if(!NativeInterop.isVirtualMachineCreated())
		{
			NativeInterop.createVirtualMachine();
		}
		Intent intent = getIntent();
		if(intent.getAction() != null)
		{
			if(intent.getAction().equals(Intent.ACTION_VIEW))
			{
				try
				{
					VirtualMachineManager.launchDisk(this, new File(intent.getData().getPath()));
					finish();
				}
				catch(Exception e)
				{
					displaySimpleMessage(e.getMessage());
				}
			}
		}
	}

	private void displaySimpleMessage(String message)
	{
		new AlertDialog.Builder(this)
				.setTitle("Error")
				.setMessage(message)
				.setPositiveButton(android.R.string.ok, (dialog, id) ->
						finish()
				)
				.setCancelable(false)
				.create()
				.show();
	}
}
