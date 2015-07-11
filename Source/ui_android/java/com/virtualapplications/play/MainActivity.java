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
import org.apache.commons.lang3.StringUtils;
import com.android.util.FileUtils;

public class MainActivity extends Activity 
{	
	private static final String PREFERENCE_CURRENT_DIRECTORY = "CurrentDirectory";
	
	private SharedPreferences _preferences;
	private FileListItem[] _fileListItems;
	private ListView _fileListView;
	private TextView _fileListViewHeader;
	private ArrayList<FileListItem> fileListItems;
	private static Activity mActivity;
	
	@Override 
	protected void onCreate(Bundle savedInstanceState) 
	{
		super.onCreate(savedInstanceState);
		//Log.w(Constants.TAG, "MainActivity - onCreate");
		
		setContentView(R.layout.main);
		
		mActivity = MainActivity.this;
		
		File filesDir = getFilesDir();
		NativeInterop.setFilesDirPath(Environment.getExternalStorageDirectory().getAbsolutePath());

		_preferences = getSharedPreferences("prefs", MODE_PRIVATE);
		EmulatorActivity.RegisterPreferences();
	
		if(!NativeInterop.isVirtualMachineCreated())
		{
			NativeInterop.createVirtualMachine();
		}
		
		prepareFileListView();
//		updateFileListView();
		
		String sdcard = getCurrentDirectory();
		
		new ImageFinder(R.array.disks).execute(sdcard);
		new ImageFinder(R.array.elves).execute(sdcard);

		if (sdcard.equals(Environment.getExternalStorageDirectory().getAbsolutePath())) {
			HashSet<String> extStorage = MainActivity.getExternalMounts();
			if (extStorage != null && !extStorage.isEmpty()) {
				for (Iterator<String> sd = extStorage.iterator(); sd.hasNext();) {
					String sdCardPath = sd.next().replace("mnt/media_rw", "storage");
					if (!sdCardPath.equals(sdcard)) {
						if (new File(sdCardPath).canRead()) {
							new ImageFinder(R.array.disks).execute(sdCardPath);
							new ImageFinder(R.array.elves).execute(sdCardPath);
						}
					}
				}
			}
		}
	}

//	@Override
//	public void onBackPressed()
//	{
//		File currentDirectoryFile = new File(getCurrentDirectory());
//		File parentDirectoryFile = currentDirectoryFile.getParentFile();
//		if(parentDirectoryFile == null) {
//			finish();
//		}
//		setCurrentDirectory(currentDirectoryFile.getParentFile().getAbsolutePath());
//		updateFileListView();
//	}

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
	
	public static HashSet<String> getExternalMounts() {
		final HashSet<String> out = new HashSet<String>();
		String reg = "(?i).*vold.*(vfat|ntfs|exfat|fat32|ext3|ext4|fuse).*rw.*";
		String s = "";
		try {
			final java.lang.Process process = new ProcessBuilder().command("mount")
			.redirectErrorStream(true).start();
			process.waitFor();
			final InputStream is = process.getInputStream();
			final byte[] buffer = new byte[1024];
			while (is.read(buffer) != -1) {
				s = s + new String(buffer);
			}
			is.close();
		} catch (final Exception e) {
			
		}
		
		final String[] lines = s.split("\n");
		for (String line : lines) {
			if (StringUtils.containsIgnoreCase(line, "secure"))
				continue;
			if (StringUtils.containsIgnoreCase(line, "asec"))
				continue;
			if (line.matches(reg)) {
				String[] parts = line.split(" ");
				for (String part : parts) {
					if (part.startsWith("/"))
						if (!StringUtils.containsIgnoreCase(part, "vold"))
							out.add(part);
				}
			}
		}
		return out;
	}
	
	private void setCurrentDirectory(String currentDirectory)
	{
		SharedPreferences.Editor preferencesEditor = _preferences.edit();
		preferencesEditor.putString(PREFERENCE_CURRENT_DIRECTORY, currentDirectory);
		preferencesEditor.commit();
	}
	
	private void clearCurrentDirectory()
	{
		SharedPreferences.Editor preferencesEditor = _preferences.edit();
		preferencesEditor.remove(PREFERENCE_CURRENT_DIRECTORY);
		preferencesEditor.commit();
	}
	
	public static void resetDirectory() {
		((MainActivity) mActivity).clearCurrentDirectory();
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
	
	private final class ImageFinder extends AsyncTask<String, Integer, List<File>> {
		
		private int array;
		
		public ImageFinder(int arrayType) {
			this.array = arrayType;
		}
		
		protected void onPreExecute() {
			_fileListView.setOnItemClickListener(
				new OnItemClickListener()
				{
					@Override
					public void onItemClick(AdapterView<?> parent, View view, int position, long id)
					{
						//Remove one to compensate for header
						position = position - 1;
						
						FileListItem fileListItem = _fileListItems[position];
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
							setCurrentDirectory(fileListItem.getPath().substring(0, fileListItem.getPath().lastIndexOf(File.separator)));
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
			);
		}
		
		@Override
		protected List<File> doInBackground(String... paths) {
			File storage = new File(paths[0]);

			String[] mediaTypes = MainActivity.this.getResources().getStringArray(array);
			FilenameFilter[] filter = new FilenameFilter[mediaTypes.length];
			
			int i = 0;
			for (final String type : mediaTypes) {
				filter[i] = new FilenameFilter() {
					
					public boolean accept(File dir, String name) {
						if (dir.getName().startsWith(".") || name.startsWith(".")) {
							return false;
						} else {
							return StringUtils.endsWithIgnoreCase(name, "." + type);
						}
					}
					
				};
				i++;
			}
			
			FileUtils fileUtils = new FileUtils();
			Collection<File> files = fileUtils.listFiles(storage, filter, -1);
			return (List<File>) files;
		}
		
		@Override
		protected void onPostExecute(List<File> images) {
			if (images != null && !images.isEmpty()) {
				// Create the list of acceptable images
				for(File file : images)
				{
					fileListItems.add(new FileListItem(file.getAbsolutePath()));
				}
				
				Collections.sort(fileListItems);

				_fileListItems = fileListItems.toArray(new FileListItem[images.size()]);
				
				ArrayAdapter<FileListItem> adapter = new ArrayAdapter<FileListItem>(MainActivity.this,
					android.R.layout.simple_list_item_1, android.R.id.text1, _fileListItems);
				_fileListView.setAdapter(adapter);
			} else {
				// Display warning that no disks exist
			}
		}
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
		
//		_fileListView.setOnItemClickListener(
//			new OnItemClickListener() 
//			{
//				@Override 
//				public void onItemClick(AdapterView<?> parent, View view, int position, long id) 
//				{
//					//Remove one to compensate for header
//					position = position - 1;
//					
//					FileListItem fileListItem = _fileListItems[position];
//					if(fileListItem.isParentDirectory())
//					{
//						File currentDirectoryFile = new File(getCurrentDirectory());
//						File parentDirectoryFile = currentDirectoryFile.getParentFile();
//						if(parentDirectoryFile == null) return;
//						setCurrentDirectory(currentDirectoryFile.getParentFile().getAbsolutePath());
//						updateFileListView();
//					}
//					else if(fileListItem.isDirectory())
//					{
//						setCurrentDirectory(fileListItem.getPath());
//						updateFileListView();
//					}
//					else
//					{
//						try
//						{
//							if(IsLoadableExecutableFileName(fileListItem.getPath()))
//							{
//								NativeInterop.loadElf(fileListItem.getPath());
//							}
//							else
//							{
//								NativeInterop.bootDiskImage(fileListItem.getPath());
//							}
//						}
//						catch(Exception ex)
//						{
//							displaySimpleMessage("Error", ex.getMessage());
//							return;
//						}
//						//TODO: Catch errors that might happen while loading files
//						Intent intent = new Intent(getApplicationContext(), EmulatorActivity.class);
//						startActivity(intent);
//					}
//				}
//			}
//		);
		
		fileListItems = new ArrayList<FileListItem>();
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
