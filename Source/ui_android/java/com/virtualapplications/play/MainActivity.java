package com.virtualapplications.play;

import android.app.*;
import android.content.*;
import android.content.pm.*;
import android.os.*;
import android.util.*;
import android.view.*;
import android.view.View;
import android.view.View.OnClickListener;
import android.view.View.OnLongClickListener;
import android.widget.*;
import android.widget.AdapterView.*;
import android.widget.GridLayout.LayoutParams;
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
	private GridLayout gameListing;
	private static Activity mActivity;
	private boolean isConfigured = false;
	private int totalItems;
	private int rowNumber;
	private int columnNumber;
	private int columnCount;
	
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
		((MainActivity) mActivity).isConfigured = false;
		((MainActivity) mActivity).prepareFileListView();
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
		
		private View createListItem(final File game, final int index) {
			final View childview = MainActivity.this.getLayoutInflater().inflate(
				R.layout.game_list_item, null, false);
			
			if (!isConfigured) {
				
				((TextView) childview.findViewById(R.id.game_text)).setText(game.getName());
				
				childview.findViewById(R.id.childview).setOnClickListener(new OnClickListener() {
					public void onClick(View view) {
						launchDisk(game);
						prepareFileListView();
						return;
					}
				});
				
				return childview;
			}
			
			final GamesDbAPI gameParser = new GamesDbAPI(game, index);
			gameParser.setViewParent(MainActivity.this, childview);
			
			childview.findViewById(R.id.childview).setOnLongClickListener(new OnLongClickListener() {
				public boolean onLongClick(View view) {
					final AlertDialog.Builder builder = new AlertDialog.Builder(MainActivity.this);
					builder.setCancelable(true);
					builder.setTitle(gameParser.getGameTitle());
					builder.setIcon(gameParser.getGameIcon());
					builder.setMessage(gameParser.getGameDetails());
					builder.setPositiveButton("Close",
											  new DialogInterface.OnClickListener() {
						public void onClick(DialogInterface dialog, int which) {
							dialog.dismiss();
							return;
						}
					});
					builder.setPositiveButton("Launch",
											  new DialogInterface.OnClickListener() {
						public void onClick(DialogInterface dialog, int which) {
							dialog.dismiss();
							launchDisk(game);
							return;
						}
					});
					builder.create().show();
					return true;
				}
			});
			
			
			childview.findViewById(R.id.childview).setOnClickListener(new OnClickListener() {
				public void onClick(View view) {
					launchDisk(game);
					return;
				}
			});
			gameParser.execute(game.getName());
			return childview;
		}
		
		protected void onPreExecute() {
			
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

				Collections.sort(images);
				
				totalItems += images.size();
				// Should be replaced by screen size calculation
				int row = totalItems / columnCount;
				gameListing.setColumnCount(columnCount);
				gameListing.setRowCount(row + 1);
				for (int i = 0; i < images.size(); i++, columnNumber++)
				{
					if (columnNumber == columnCount)
					{
						columnNumber = 0;
						rowNumber++;
					}
					View view = createListItem(images.get(i), i);
					GridLayout.LayoutParams param =new GridLayout.LayoutParams();
					param.height = LayoutParams.WRAP_CONTENT;
					param.width = LayoutParams.WRAP_CONTENT;
					param.rightMargin = 5;
					param.topMargin = 5;
					param.setGravity(Gravity.CENTER);
					param.columnSpec = GridLayout.spec(columnNumber);
					param.rowSpec = GridLayout.spec(rowNumber);
					view.setLayoutParams (param);
					gameListing.addView(view);
				}
//				gameListing.invalidate();
			} else {
				// Display warning that no disks exist
			}
		}
	}
	
	private void launchDisk (File game) {
		try
		{
			if(IsLoadableExecutableFileName(game.getPath()))
			{
				NativeInterop.loadElf(game.getPath());
			}
			else
			{
				NativeInterop.bootDiskImage(game.getPath());
			}
			setCurrentDirectory(game.getPath().substring(0, game.getPath().lastIndexOf(File.separator)));
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

	private void prepareFileListView()
	{
		gameListing = (GridLayout) findViewById(R.id.game_grid);
		if (gameListing != null) {
			gameListing.removeAllViews();
		}
		
		totalItems = 0;
		rowNumber = 0;
		columnNumber = 0;
		
		DisplayMetrics metrics = new DisplayMetrics();
		getWindowManager().getDefaultDisplay().getMetrics(metrics);
		final float scale = getResources().getDisplayMetrics().density;
		int screenWidth = (int) (metrics.widthPixels * scale + 0.5f);
		columnCount = (int) ((screenWidth / 180) / 3) - 1;
		// Divide screen width by a single column, then by columnSpan
		
		String sdcard = getCurrentDirectory();
		if (!sdcard.equals(Environment.getExternalStorageDirectory().getAbsolutePath())) {
			isConfigured = true;
		}
		
		new ImageFinder(R.array.disks).execute(sdcard);
		new ImageFinder(R.array.elves).execute(sdcard);
		
		if (!isConfigured) {
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
}
