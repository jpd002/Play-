package com.virtualapplications.play;

import android.app.AlertDialog;
import android.app.ProgressDialog;
import android.content.Context;
import android.net.Uri;
import android.os.AsyncTask;
import android.widget.Toast;

public final class DataFilesImpExpProcessTask extends AsyncTask<Void, Void, Void>
{
	ProgressDialog _progDialog;
	final Context _context;
	final DataFilesImpExpProcess _process;
	final DataFilesImpExpProcess.ImpExpAction _action;
	final Uri _targetFolderUri;
	final String _taskTitleText;
	final String _taskCompleteText;
	Exception _exception;

	public DataFilesImpExpProcessTask(Context context, DataFilesImpExpProcess.ImpExpAction action, Uri targetFolderUri)
	{
		_context = context;
		_action = action;
		_targetFolderUri = targetFolderUri;
		switch(action)
		{
		case Import:
			_taskTitleText = _context.getString(R.string.datafiles_import_title);
			_taskCompleteText = _context.getString(R.string.datafiles_import_complete);
			break;
		case Export:
			_taskTitleText = _context.getString(R.string.datafiles_export_title);
			_taskCompleteText = _context.getString(R.string.datafiles_export_complete);
			break;
		default:
			_taskTitleText = "";
			_taskCompleteText = "";
			break;
		}
		_process = new DataFilesImpExpProcess(context);
	}

	protected void onPreExecute()
	{
		_progDialog = ProgressDialog.show(_context,
				_taskTitleText,
				_context.getString(R.string.datafiles_impexp_task_msg), true);
	}

	@Override
	protected Void doInBackground(Void... params)
	{
		try
		{
			_process.Start(_action, _targetFolderUri);
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
					.setTitle(_taskTitleText)
					.setMessage(String.format("Failed: %s", _exception.getMessage()))
					.create()
					.show();
		}
		else
		{
			Toast.makeText(_context, _taskCompleteText, Toast.LENGTH_SHORT).show();
		}
	}
}
