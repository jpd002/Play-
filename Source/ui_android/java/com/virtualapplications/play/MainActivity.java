package com.virtualapplications.play;

import android.app.*;
import android.app.ProgressDialog;
import android.content.*;
import android.content.pm.*;
import android.content.res.Configuration;
import android.graphics.drawable.BitmapDrawable;
import android.os.*;
import android.util.*;
import android.view.*;
import android.view.View;
import android.view.View.OnClickListener;
import android.view.View.OnLongClickListener;
import android.widget.*;
import android.widget.TableLayout;
import android.widget.TableRow;
import android.widget.TextView;
import android.support.v4.app.ActionBarDrawerToggle;
import android.support.v4.widget.DrawerLayout;
import java.io.*;
import java.text.*;
import java.util.*;
import java.util.concurrent.ExecutionException;
import java.util.zip.*;
import org.apache.commons.lang3.StringUtils;
import com.android.util.FileUtils;

import static org.apache.commons.lang3.text.WordUtils.capitalizeFully;

public class MainActivity extends Activity 
{	
	private static final String PREFERENCE_CURRENT_DIRECTORY = "CurrentDirectory";
	
	private SharedPreferences _preferences;
	private TableLayout gameListing;
	private static Activity mActivity;
	private boolean isConfigured = false;
	private int numColumn = 0;
	private DrawerLayout mDrawerLayout;
	private ActionBarDrawerToggle mDrawerToggle;
	private float localScale;
	private int currentOrientation;
	
	@Override 
	protected void onCreate(Bundle savedInstanceState) 
	{
		super.onCreate(savedInstanceState);
		//Log.w(Constants.TAG, "MainActivity - onCreate");
		
		currentOrientation = getResources().getConfiguration().orientation;
		if (currentOrientation == Configuration.ORIENTATION_LANDSCAPE) {
			setRequestedOrientation(ActivityInfo.SCREEN_ORIENTATION_SENSOR_LANDSCAPE);
		} else {
			setRequestedOrientation(ActivityInfo.SCREEN_ORIENTATION_SENSOR_PORTRAIT);
		}
		
		setContentView(R.layout.main);
		
		mDrawerLayout = (DrawerLayout) findViewById(R.id.drawer_layout);
		mDrawerToggle = new ActionBarDrawerToggle(
			MainActivity.this,      /* host Activity */
			mDrawerLayout,			/* DrawerLayout object */
			R.drawable.ic_drawer,	/* nav drawer icon to replace 'Up' caret */
			R.string.drawer_open,	/* "open drawer" description */
			R.string.drawer_shut	/* "close drawer" description */
		) {
			/** Called when a drawer has settled in a completely closed state. */
			public void onDrawerClosed(View view) {
				super.onDrawerClosed(view);
				if (!isConfigured) {
					getActionBar().setTitle(getString(R.string.menu_title_look));
				} else {
					getActionBar().setTitle(getString(R.string.menu_title_shut));
				}
			}
			
			/** Called when a drawer has settled in a completely open state. */
			public void onDrawerOpened(View drawerView) {
				super.onDrawerOpened(drawerView);
				getActionBar().setTitle(getString(R.string.menu_title_open));
			}
		};
		
		// Set the drawer toggle as the DrawerListener
		mDrawerLayout.setDrawerListener(mDrawerToggle);
		
		Button buttonPrefs = (Button) mDrawerLayout.findViewById(R.id.main_menu_settings);
		buttonPrefs.setOnClickListener(new OnClickListener() {
			@Override
			public void onClick(View arg0) {
				displaySettingsActivity();
			}
		});
		
		Button buttonAbout = (Button) mDrawerLayout.findViewById(R.id.main_menu_about);
		buttonAbout.setOnClickListener(new OnClickListener() {
			@Override
			public void onClick(View arg0) {
				displayAboutDialog();
			}
		});
		
		getActionBar().setDisplayHomeAsUpEnabled(true);
		getActionBar().setHomeButtonEnabled(true);
		getActionBar().setTitle(getString(R.string.menu_title_shut));
		
		mActivity = MainActivity.this;
		
		File filesDir = getFilesDir();
		NativeInterop.setFilesDirPath(Environment.getExternalStorageDirectory().getAbsolutePath());

		_preferences = getSharedPreferences("prefs", MODE_PRIVATE);
		EmulatorActivity.RegisterPreferences();
	
		if(!NativeInterop.isVirtualMachineCreated())
		{
			NativeInterop.createVirtualMachine();
		}

		initialDbSetup();
		prepareFileListView();
	}

	private void initialDbSetup() {
		try {
			new SetupDB().execute(this).get();
		} catch (InterruptedException e) {
			e.printStackTrace();
		} catch (ExecutionException e) {
			e.printStackTrace();
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
	protected void onPostCreate(Bundle savedInstanceState) {
		super.onPostCreate(savedInstanceState);
		// Sync the toggle state after onRestoreInstanceState has occurred.
		mDrawerToggle.syncState();
	}
	
	@Override
	public void onConfigurationChanged(Configuration newConfig) {
		super.onConfigurationChanged(newConfig);
		mDrawerToggle.onConfigurationChanged(newConfig);
		
	}
	
//	@Override
//	public boolean onCreateOptionsMenu(Menu menu)
//	{
//		MenuInflater inflater = getMenuInflater();
//		inflater.inflate(R.layout.main_menu, menu);
//		return super.onCreateOptionsMenu(menu);
//	}
	
	@Override
	public boolean onOptionsItemSelected(MenuItem item) 
	{
		// Pass the event to ActionBarDrawerToggle, if it returns
		// true, then it has handled the app icon touch event
		if (mDrawerToggle.onOptionsItemSelected(item)) {
			return true;
		}
		switch(item.getItemId()) 
		{
		case android.R.id.home:
			if (mDrawerLayout.isDrawerOpen(Gravity.LEFT)) {
				mDrawerLayout.closeDrawer(Gravity.LEFT);
			} else {
				mDrawerLayout.openDrawer(Gravity.LEFT);
			}
			return true;
//		case R.id.main_menu_settings:
//			displaySettingsActivity();
//			return true;
//		case R.id.main_menu_about:
//			displayAboutDialog();
//			return true;
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
		private ProgressDialog progDialog;
		
		public ImageFinder(int arrayType) {
			this.array = arrayType;
		}
		
		private List<File> getFileList(String path) {
			File storage = new File(path);
			
			String[] mediaTypes = MainActivity.this.getResources().getStringArray(array);
			FilenameFilter[] filter = new FilenameFilter[mediaTypes.length];
			
			int i = 0;
			for (final String type : mediaTypes) {
				filter[i] = new FilenameFilter() {
					
					public boolean accept(File dir, String name) {
						if (dir.getName().startsWith(".") || name.startsWith(".")) {
							return false;
						} else if (StringUtils.endsWithIgnoreCase(name, "." + type)) {
							File disk = new File(dir, name);
							String serial = getGameDetails.getSerial(disk);
							return IsLoadableExecutableFileName(disk.getPath()) ||
								(serial != null && !serial.equals(""));
						} else {
							return false;
						}
					}
					
				};
				i++;
			}
			FileUtils fileUtils = new FileUtils();
			Collection<File> files = fileUtils.listFiles(storage, filter, -1);
			return (List<File>) files;
		}
		
		private View createListItem(final File game) {
			
			if (!isConfigured) {
				
				final View childview = MainActivity.this.getLayoutInflater().inflate(
					R.layout.file_list_item, null, false);
				
				((TextView) childview.findViewById(R.id.game_text)).setText(game.getName());
				
				childview.findViewById(R.id.childview).setOnClickListener(new OnClickListener() {
					public void onClick(View view) {
						setCurrentDirectory(game.getPath().substring(0,
							game.getPath().lastIndexOf(File.separator)));
						isConfigured = true;
						prepareFileListView();
						return;
					}
				});
				
				return childview;
			}
			
			final View childview = MainActivity.this.getLayoutInflater().inflate(
				R.layout.game_list_item, null, false);

			final getGameDetails GG = new getGameDetails(game, mActivity);
			GG.setViewParent(childview);

			childview.findViewById(R.id.childview).setOnLongClickListener(new OnLongClickListener() {
				public boolean onLongClick(View view) {
					final AlertDialog.Builder builder = new AlertDialog.Builder(MainActivity.this);
					builder.setCancelable(true);
					builder.setTitle(capitalizeFully(GG.GameInfo.getName()));
					builder.setIcon(new BitmapDrawable(GG.GameInfo.getBackCover()));
					builder.setMessage(GG.GameInfo.getDescription(mActivity));
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
			GG.execute(game.getName());
			return childview;
		}
		
		protected void onPreExecute() {
			progDialog = ProgressDialog.show(MainActivity.this,
				getString(R.string.search_games),
				getString(R.string.search_games_msg), true);
		}
		
		@Override
		protected List<File> doInBackground(String... paths) {
			
			final String root_path = paths[0];
			ArrayList<File> files = new ArrayList<File>();
			files.addAll(getFileList(root_path));
			
			if (!isConfigured) {
				HashSet<String> extStorage = MainActivity.getExternalMounts();
				if (extStorage != null && !extStorage.isEmpty()) {
					for (Iterator<String> sd = extStorage.iterator(); sd.hasNext();) {
						String sdCardPath = sd.next().replace("mnt/media_rw", "storage");
						if (!sdCardPath.equals(root_path)) {
							if (new File(sdCardPath).canRead()) {
								files.addAll(getFileList(sdCardPath));
							}
						}
					}
				}
			}
			
			
			return (List<File>) files;
		}
		
		@Override
		protected void onPostExecute(List<File> images) {
			if (progDialog != null && progDialog.isShowing()) {
				progDialog.dismiss();
			}
			if (images != null && !images.isEmpty()) {
				// Create the list of acceptable images

				Collections.sort(images);
	
				TableRow game_row = new TableRow(MainActivity.this);
				if (isConfigured) {
					game_row.setGravity(Gravity.CENTER);
				}
				int pad = (int) (10 * localScale + 0.5f);
				game_row.setPadding(0, 0, 0, pad);
				
				if (!isConfigured) {
					TableRow.LayoutParams params = new TableRow.LayoutParams(
						TableRow.LayoutParams.MATCH_PARENT,
						TableRow.LayoutParams.WRAP_CONTENT);
					params.gravity = Gravity.CENTER_VERTICAL;

					for (int i = 0; i < images.size(); i++)
					{
						game_row.addView(createListItem(images.get(i)));
						gameListing.addView(game_row, params);
						game_row = new TableRow(MainActivity.this);
						game_row.setPadding(0, 0, 0, pad);
					}
				} else {
					TableRow.LayoutParams params = new TableRow.LayoutParams(
						TableRow.LayoutParams.WRAP_CONTENT,
						TableRow.LayoutParams.WRAP_CONTENT);
					params.gravity = Gravity.CENTER;

					int column = 0;
					for (int i = 0; i < images.size(); i++)
					{
						if (column == numColumn)
						{
							gameListing.addView(game_row, params);
							column = 0;
							game_row = new TableRow(MainActivity.this);
							game_row.setGravity(Gravity.CENTER);
							game_row.setPadding(0, 0, 0, pad);
						}
						game_row.addView(createListItem(images.get(i)));
						column ++;
					}
					if (column != 0) {
						gameListing.addView(game_row, params);
					}
				}
				gameListing.invalidate();
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
	
	private boolean isAndroidTV(Context context) {
		if (Build.VERSION.SDK_INT >= Build.VERSION_CODES.ICE_CREAM_SANDWICH) {
			UiModeManager uiModeManager = (UiModeManager)
			context.getSystemService(Context.UI_MODE_SERVICE);
			if (uiModeManager.getCurrentModeType() == Configuration.UI_MODE_TYPE_TELEVISION) {
				return true;
			}
		}
		return false;
	}

	private void prepareFileListView()
	{
		gameListing = (TableLayout) findViewById(R.id.game_grid);
		if (gameListing != null) {
			gameListing.removeAllViews();
		}
		
		DisplayMetrics metrics = new DisplayMetrics();
		getWindowManager().getDefaultDisplay().getMetrics(metrics);
		localScale = getResources().getDisplayMetrics().density;
		int screenWidth = (int) (metrics.widthPixels * localScale + 0.5f);
		int screenHeight = (int) (metrics.heightPixels * localScale + 0.5f);
		
		if (screenWidth > screenHeight) {
			numColumn = 3;
		} else {
			numColumn = 2;
		}
		
		if (isAndroidTV(getApplicationContext())) {
			numColumn += 1;
		}
		
		String sdcard = getCurrentDirectory();
		if (!sdcard.equals(Environment.getExternalStorageDirectory().getAbsolutePath())) {
			isConfigured = true;
		}
		
		new ImageFinder(R.array.disks).execute(sdcard);
		
		if (!mDrawerLayout.isDrawerOpen(Gravity.LEFT)) {
			if (!isConfigured) {
				getActionBar().setTitle(getString(R.string.menu_title_look));
			} else {
				getActionBar().setTitle(getString(R.string.menu_title_shut));
			}
		}
	}
}
