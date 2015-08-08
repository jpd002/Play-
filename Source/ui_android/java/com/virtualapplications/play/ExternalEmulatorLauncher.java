package com.virtualapplications.play;

import android.app.Activity;
import android.app.AlertDialog;
import android.content.DialogInterface;
import android.content.Intent;
import android.os.Bundle;
import android.os.Environment;
import android.util.Log;
import android.view.ContextThemeWrapper;

import java.io.File;

public class ExternalEmulatorLauncher extends Activity {
    private boolean isMSG = false;

    public ExternalEmulatorLauncher() {
    }

    @Override
    public void onCreate(Bundle savedInstanceState) {
        super.onCreate(savedInstanceState);
        // TODO: This method is called when the BroadcastReceiver is receiving
        // an Intent broadcast.
        NativeInterop.setFilesDirPath(Environment.getExternalStorageDirectory().getAbsolutePath());

        EmulatorActivity.RegisterPreferences();

        if(!NativeInterop.isVirtualMachineCreated())
        {
            NativeInterop.createVirtualMachine();
        }
        Intent intent = getIntent();
        if (intent.getAction() != null) {
            if (intent.getAction().equals(Intent.ACTION_VIEW)) {
                launchDisk(new File(intent.getData().getPath()));
            }
        }
        if (!isMSG)
        finish();
    }

    private void launchDisk (File game) {
        try
        {
            if(MainActivity.IsLoadableExecutableFileName(game.getPath()))
            {
                game.setLastModified(System.currentTimeMillis());
                NativeInterop.loadElf(game.getPath());
            }
            else
            {
                    game.setLastModified(System.currentTimeMillis());
                    NativeInterop.bootDiskImage(game.getPath());
            }
        }
        catch(Exception ex)
        {
            isMSG = true;
            displaySimpleMessage("Error", ex.getMessage());
            return;
        }
        //TODO: Catch errors that might happen while loading files
        Intent intent = new Intent(getApplicationContext(), EmulatorActivity.class);
        startActivity(intent);
    }

    private void displaySimpleMessage(String title, String message)
    {
        AlertDialog.Builder builder = new AlertDialog.Builder(this);

        builder.setTitle(title);
        builder.setMessage(message);

        builder.setPositiveButton("OK",
                new DialogInterface.OnClickListener()
                {
                    @Override
                    public void onClick(DialogInterface dialog, int id)
                    {
                        finish();
                    }
                }
        )
        .setCancelable(false);

        AlertDialog dialog = builder.create();
        dialog.show();
    }
}
