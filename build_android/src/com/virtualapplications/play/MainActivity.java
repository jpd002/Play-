package com.virtualapplications.play;

import android.app.Activity;
import android.os.Bundle;
import android.util.Log;
import android.view.*;
import android.view.View.*;
import android.widget.*;
import java.io.*;

public class MainActivity extends Activity 
{
	@Override protected void onCreate(Bundle icicle) 
	{
		super.onCreate(icicle);
		setContentView(R.layout.main);
		
		File filesDir = getFilesDir();
		NativeInterop.setFilesDirPath(filesDir.getAbsolutePath());
		
		NativeInterop.createVirtualMachine();
		
		((Button)findViewById(R.id.startTests)).setOnClickListener(
			new OnClickListener() 
			{
				public void onClick(View view) 
				{
					NativeInterop.start();
				}
			}
		);
	}
}
