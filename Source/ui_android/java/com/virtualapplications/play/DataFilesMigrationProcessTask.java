package com.virtualapplications.play;

import android.app.AlertDialog;
import android.app.ProgressDialog;
import android.content.Context;
import android.net.Uri;
import android.os.AsyncTask;
import android.widget.Toast;

public final class DataFilesMigrationProcessTask extends AsyncTask<Void, Void, Void>
{
	ProgressDialog _progDialog;
	final Context _context;
	final DataFilesMigrationProcess _process;
	final Uri _dataFilesFolderUri;
	Exception _exception;

	public DataFilesMigrationProcessTask(Context context, Uri dataFilesFolderUri)
	{
		_context = context;
		_dataFilesFolderUri = dataFilesFolderUri;
		_process = new DataFilesMigrationProcess(context);
	}

	protected void onPreExecute()
	{
		_progDialog = ProgressDialog.show(_context,
				_context.getString(R.string.migration_title),
				_context.getString(R.string.migration_task_msg), true);
	}

	@Override
	protected Void doInBackground(Void... params)
	{
		try
		{
			_process.Start(_dataFilesFolderUri);
		}
		catch(Exception ex)
		{
			_exception = ex;
		}
		return null;
	}

	@Override
	protected void onPostExecute(Void result)
	{
		if(_progDialog != null && _progDialog.isShowing())
		{
			try
			{
				_progDialog.dismiss();
			}
			catch(final Exception e)
			{
				//We don't really care if we get an exception while dismissing
			}
		}
		if(_exception != null)
		{
			new AlertDialog.Builder(_context)
					.setTitle(_context.getString(R.string.migration_title))
					.setMessage(String.format("Failed: %s", _exception.getMessage()))
					.create()
					.show();
		}
		else
		{
			Toast.makeText(_context, "Migration complete.", Toast.LENGTH_SHORT).show();
		}
	}
}
