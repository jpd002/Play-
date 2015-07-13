package com.virtualapplications.play;

import android.app.*;
import android.content.*;
import android.content.pm.*;
import android.os.*;
import android.util.*;
import android.view.*;
import android.widget.*;
import android.widget.AdapterView.*;
import java.io.*;
import java.text.*;
import java.util.*;
import java.util.zip.*;

public class MainActivity extends Activity 
{	
	private static final String PREFERENCE_CURRENT_DIRECTORY = "CurrentDirectory";
	
	private SharedPreferences _preferences;
	private FileListItem[] _fileListItems;
	private ListView _fileListView;
	private TextView _fileListViewHeader;
	
	@Override 
	protected void onCreate(Bundle savedInstanceState) 
	{
		super.onCreate(savedInstanceState);
		//Log.w(Constants.TAG, "MainActivity - onCreate");
		
		setContentView(R.layout.main);
		
		File filesDir = getFilesDir();
		NativeInterop.setFilesDirPath(Environment.getExternalStorageDirectory().getAbsolutePath());

		_preferences = getSharedPreferences("prefs", MODE_PRIVATE);
		EmulatorActivity.RegisterPreferences();
	
		if(!NativeInterop.isVirtualMachineCreated())
		{
			NativeInterop.createVirtualMachine();
		}
		
		prepareFileListView();
		updateFileListView();
	}

	@Override
	public void onBackPressed()
	{
		File currentDirectoryFile = new File(getCurrentDirectory());
		File parentDirectoryFile = currentDirectoryFile.getParentFile();
		if(parentDirectoryFile == null) {
			finish();
		} else {
			setCurrentDirectory(currentDirectoryFile.getParentFile().getAbsolutePath());
			updateFileListView();
		}
		
	}

	private static long getBuildDate(Context context) 
	{
		try
		{
			ApplicationInfo ai = context.getPackageManager().getApplicationInfo(context.getPackageName(), 0);
			ZipFile zf = new ZipFile(ai.sourceDir);
			ZipEntry ze = zf.getEntry("classes.dex");
			long time = ze.getTime();
			return time;
		} 
		catch (Exception e) 
		{

		}
		return 0;
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
					
				}
			}
		);
		
		AlertDialog dialog = builder.create();
		dialog.show();
	}
	
	private void displaySettingsActivity()
	{
		Intent intent = new Intent(getApplicationContext(), SettingsActivity.class);
		startActivity(intent);
	}
	
	private void displayAboutDialog()
	{
		long buildDate = getBuildDate(this);
		String buildDateString = new SimpleDateFormat("yyyy/MM/dd", Locale.getDefault()).format(buildDate);
		String aboutMessage = String.format("Build Date: %s", buildDateString);
		displaySimpleMessage("About Play!", aboutMessage);
	}
	
	@Override
	public boolean onCreateOptionsMenu(Menu menu) 
	{
		MenuInflater inflater = getMenuInflater();
		inflater.inflate(R.layout.main_menu, menu);
		return super.onCreateOptionsMenu(menu);
	}
	
	@Override
	public boolean onOptionsItemSelected(MenuItem item) 
	{
		switch(item.getItemId()) 
		{
		case R.id.main_menu_settings:
			displaySettingsActivity();
			return true;
		case R.id.main_menu_about:
			displayAboutDialog();
			return true;
		case R.id.main_menu_exit:
			finish();
			return true;
		default:
			return super.onOptionsItemSelected(item);
		}
	}
	
	private String getCurrentDirectory()
	{
		return _preferences.getString(PREFERENCE_CURRENT_DIRECTORY, Environment.getExternalStorageDirectory().getAbsolutePath());
	}
	
	private void setCurrentDirectory(String currentDirectory)
	{
		SharedPreferences.Editor preferencesEditor = _preferences.edit();
		preferencesEditor.putString(PREFERENCE_CURRENT_DIRECTORY, currentDirectory);
		preferencesEditor.commit();
	}
	
	private static boolean IsLoadableExecutableFileName(String fileName)
	{
		return fileName.toLowerCase().endsWith(".elf");
	}
	
	private static boolean IsLoadableDiskImageFileName(String fileName)
	{
		
		return  
				fileName.toLowerCase().endsWith(".iso") ||
				fileName.toLowerCase().endsWith(".bin") ||
				fileName.toLowerCase().endsWith(".cso") ||
				fileName.toLowerCase().endsWith(".isz");
	}
	
	private static FileListItem[] getFileList(String directoryPath)
	{
		ArrayList<FileListItem> fileListItems = new ArrayList<FileListItem>();
		fileListItems.add(new FileListItem(".."));
		File directoryFile = new File(directoryPath);
		File[] fileList = directoryFile.listFiles();
		if(fileList != null)
		{
			for(File file : fileList)
			{
				if(file.isDirectory())
				{
					fileListItems.add(new FileListItem(file.getAbsolutePath()));
				}
				else if(IsLoadableExecutableFileName(file.getName()))
				{
					fileListItems.add(new FileListItem(file.getAbsolutePath()));
				}
				else if(IsLoadableDiskImageFileName(file.getName()))
				{
					fileListItems.add(new FileListItem(file.getAbsolutePath()));
				}
			}
		}
		Collections.sort(fileListItems);
		return fileListItems.toArray(new FileListItem[fileListItems.size()]);
	}
	
	private void prepareFileListView()
	{
		_fileListView = (ListView)findViewById(R.id.main_fileList);
		
		View header = (View)getLayoutInflater().inflate(R.layout.fileview_header, null);
		_fileListViewHeader = (TextView)header.findViewById(R.id.headerTextView);

		_fileListView.addHeaderView(header, null, false);
		
		_fileListView.setOnItemClickListener(
			new OnItemClickListener() 
			{
				@Override 
				public void onItemClick(AdapterView<?> parent, View view, int position, long id) 
				{
					//Remove one to compensate for header
					position = position - 1;
					
					FileListItem fileListItem = _fileListItems[position];
					if(fileListItem.isParentDirectory())
					{
						File currentDirectoryFile = new File(getCurrentDirectory());
						File parentDirectoryFile = currentDirectoryFile.getParentFile();
						if(parentDirectoryFile == null) return;
						setCurrentDirectory(currentDirectoryFile.getParentFile().getAbsolutePath());
						updateFileListView();
					}
					else if(fileListItem.isDirectory())
					{
						setCurrentDirectory(fileListItem.getPath());
						updateFileListView();
					}
					else
					{
						try
						{
							if(IsLoadableExecutableFileName(fileListItem.getPath()))
							{
								NativeInterop.loadElf(fileListItem.getPath());
							}
							else
							{
								NativeInterop.bootDiskImage(fileListItem.getPath());
							}
						}
						catch(Exception ex)
						{
							displaySimpleMessage("Error", ex.getMessage());
							return;
						}
						//TODO: Catch errors that might happen while loading files
						Intent intent = new Intent(getApplicationContext(), EmulatorActivity.class);
						startActivity(intent);
					}
				}
			}
		);
	}
	
	private void updateFileListView()
	{
		_fileListItems = getFileList(getCurrentDirectory());
		_fileListViewHeader.setText(getCurrentDirectory());
		
		ArrayAdapter<FileListItem> adapter = new ArrayAdapter<FileListItem>(this,
			android.R.layout.simple_list_item_1, android.R.id.text1, _fileListItems);
		_fileListView.setAdapter(adapter);
	}
}
