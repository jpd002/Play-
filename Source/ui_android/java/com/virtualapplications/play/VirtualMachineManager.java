package com.virtualapplications.play;

import android.app.AlertDialog;
import android.app.ProgressDialog;
import android.content.Context;
import android.content.Intent;
import android.os.AsyncTask;

public class VirtualMachineManager
{
	public interface OnGameLaunchCompleteListener
	{
		void onComplete();
	}

	private static final class GameLauncher extends AsyncTask<Void, Void, Void>
	{
		private ProgressDialog progDialog;
		final Context _ctx;
		final String _bootablePath;
		final OnGameLaunchCompleteListener _completeListener;
		Exception _exception;

		public GameLauncher(Context ctx, String bootablePath, OnGameLaunchCompleteListener completeListener)
		{
			_ctx = ctx;
			_bootablePath = bootablePath;
			_completeListener = completeListener;
		}

		protected void onPreExecute()
		{
			progDialog = ProgressDialog.show(_ctx,
					_ctx.getString(R.string.launch_game),
					_ctx.getString(R.string.launch_game_msg), true);
		}

		@Override
		protected Void doInBackground(Void... params)
		{
			try
			{
				if(BootablesInterop.IsBootableExecutablePath(_bootablePath))
				{
					NativeInterop.loadElf(_bootablePath);
				}
				else
				{
					NativeInterop.bootDiskImage(_bootablePath);
				}
				Intent intent = new Intent(_ctx, EmulatorActivity.class);
				_ctx.startActivity(intent);
			}
			catch(Exception e)
			{
				_exception = e;
			}
			return null;
		}

		@Override
		protected void onPostExecute(Void result)
		{
			if(progDialog != null && progDialog.isShowing())
			{
				try
				{
					progDialog.dismiss();
				}
				catch(final Exception e)
				{
					//We don't really care if we get an exception while dismissing
				}
			}
			if(_exception != null)
			{
				displaySimpleMessage("Error", _exception.getMessage());
			}
			else
			{
				if(_completeListener != null)
				{
					_completeListener.onComplete();
				}
			}
		}

		private void displaySimpleMessage(String title, String message)
		{
			new AlertDialog.Builder(_ctx)
					.setTitle(title)
					.setMessage(message)
					.setPositiveButton(android.R.string.ok, null)
					.setOnDismissListener(dialogInterface -> {
						if(_completeListener != null)
						{
							_completeListener.onComplete();
						}
					})
					.create()
					.show();
		}
	}

	public static void launchGame(Context ctx, String bootablePath, OnGameLaunchCompleteListener completeListener)
	{
		GameLauncher launcher = new GameLauncher(ctx, bootablePath, completeListener);
		launcher.execute();
	}
}
