package com.virtualapplications.play;

import android.app.*;
import android.app.ProgressDialog;
import android.content.*;
import android.content.pm.*;
import android.content.res.Configuration;
import android.content.res.TypedArray;
import android.graphics.Color;
import android.graphics.drawable.GradientDrawable;
import android.os.*;
import android.support.v7.app.ActionBarActivity;
import android.support.v7.widget.Toolbar;
import android.view.*;
import android.view.View;
import android.view.View.OnClickListener;
import android.widget.*;
import android.widget.GridView;
import android.widget.TextView;
import android.support.v4.widget.DrawerLayout;
import java.io.*;
import java.lang.reflect.InvocationTargetException;
import java.text.*;
import java.util.*;
import java.util.zip.*;
import org.apache.commons.io.comparator.CompositeFileComparator;
import org.apache.commons.io.comparator.LastModifiedFileComparator;
import org.apache.commons.io.comparator.SizeFileComparator;
import org.apache.commons.lang3.StringUtils;
import com.android.util.FileUtils;
import android.graphics.Point;

import com.virtualapplications.play.database.GameInfo;
import com.virtualapplications.play.database.SqliteHelper.Games;

public class MainActivity extends ActionBarActivity implements NavigationDrawerFragment.NavigationDrawerCallbacks
{	
	private static final String PREFERENCE_CURRENT_DIRECTORY = "CurrentDirectory";
	
	private SharedPreferences _preferences;
	static Activity mActivity;
	private boolean isConfigured = false;
	private int currentOrientation;
	private GameInfo gameInfo;
	protected NavigationDrawerFragment mNavigationDrawerFragment;

	private List<File> currentGames = new ArrayList<File>();
	
	public static final int SORT_RECENT = 0;
	public static final int SORT_HOMEBREW = 1;
	public static final int SORT_NONE = 2;
	private int sortMethod = SORT_NONE;

	@Override 
	protected void onCreate(Bundle savedInstanceState) 
	{
		super.onCreate(savedInstanceState);
		//Log.w(Constants.TAG, "MainActivity - onCreate");

		_preferences = getSharedPreferences("prefs", MODE_PRIVATE);
		currentOrientation = getResources().getConfiguration().orientation;

		SettingsActivity.ChangeTheme(null,this);
		if (isAndroidTV(this)) {
			setContentView(R.layout.tele);
		} else {
			setContentView(R.layout.main);
		}
		mActivity = MainActivity.this;

		NativeInterop.setFilesDirPath(Environment.getExternalStorageDirectory().getAbsolutePath());

		EmulatorActivity.RegisterPreferences();

		if(!NativeInterop.isVirtualMachineCreated())
		{
			NativeInterop.createVirtualMachine();
		}

		Intent intent = getIntent();
		if (intent.getAction() != null) {
			if (intent.getAction().equals(Intent.ACTION_VIEW)) {
				launchDisk(new File(intent.getData().getPath()), true);
				getIntent().setData(null);
				setIntent(null);
			}
		}

//		if (isAndroidTV(this)) {
//			// Load the menus for Android TV
//		} else {
			Toolbar toolbar = getSupportToolbar();
			setSupportActionBar(toolbar);
			toolbar.bringToFront();

			mNavigationDrawerFragment = (NavigationDrawerFragment)
					getFragmentManager().findFragmentById(R.id.navigation_drawer);

			// Set up the drawer.
			mNavigationDrawerFragment.setUp(
					R.id.navigation_drawer,
					(DrawerLayout) findViewById(R.id.drawer_layout));

			TypedArray a = getTheme().obtainStyledAttributes(new int[]{R.attr.colorPrimaryDark});
			int attributeResourceId = a.getColor(0, 0);
			a.recycle();
			findViewById(R.id.navigation_drawer).setBackgroundColor(Color.parseColor(
					("#" + Integer.toHexString(attributeResourceId)).replace("#ff", "#8e")
			));
//		}

		gameInfo = new GameInfo(MainActivity.this);
		getContentResolver().call(Games.GAMES_URI, "importDb", null, null);

		prepareFileListView(false);
	}

	private Toolbar getSupportToolbar() {

		View toolbar = findViewById(R.id.my_awesome_toolbar);
		final LinearLayout content = (LinearLayout) findViewById(R.id.content_frame);

		setUIcolor(content);

		return (Toolbar) toolbar;
	}

	private void setUIcolor(LinearLayout content){
		if (content != null) {
			int[] colors = new int[2];// you can increase array size to add more colors to gradient.
			TypedArray a = getTheme().obtainStyledAttributes(new int[]{R.attr.colorPrimary});
			int topgradientcolor = a.getColor(0, 0);

			a.recycle();
			float[] hsv = new float[3];
			Color.colorToHSV(topgradientcolor, hsv);
			hsv[2] *= 0.8f;// make it darker
			colors[0] = Color.HSVToColor(hsv);
			/*
			using this will blend the top of the gradient with actionbar (aka using the same color)
			colors[0] = topgradientcolor
			 */
			colors[1] = Color.rgb(20, 20, 20);
			GradientDrawable gradientbg = new GradientDrawable(GradientDrawable.Orientation.TOP_BOTTOM, colors);
			content.setBackground(gradientbg);
		}
		TypedArray a = getTheme().obtainStyledAttributes(new int[]{R.attr.colorPrimaryDark});
		int attributeResourceId = a.getColor(0, 0);
		a.recycle();
		findViewById(R.id.navigation_drawer).setBackgroundColor(Color.parseColor(
				("#" + Integer.toHexString(attributeResourceId)).replace("#ff", "#8e")
		));
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
		startActivityForResult(intent, 0);

	}

	protected void onActivityResult(int requestCode, int resultCode, Intent data) {
		if (requestCode == 0) {
			SettingsActivity.ChangeTheme(null, this);
			setUIcolor((LinearLayout) findViewById(R.id.content_frame));
		}
	}


	private void displayAboutDialog()
	{
		long buildDate = getBuildDate(this);
		String buildDateString = new SimpleDateFormat("yyyy/MM/dd", Locale.getDefault()).format(buildDate);
		String aboutMessage = String.format("Build Date: %s", buildDateString);
		displaySimpleMessage("About Play!", aboutMessage);
	}
	

	
	@Override
	public void onConfigurationChanged(Configuration newConfig) {
		super.onConfigurationChanged(newConfig);
		mNavigationDrawerFragment.onConfigurationChanged(newConfig);
		if (newConfig.orientation != currentOrientation) {
			currentOrientation = newConfig.orientation;
			getSupportToolbar();
			if (currentGames != null && !currentGames.isEmpty()) {
				prepareFileListView(true);
			} else {
				prepareFileListView(false);
			}
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
		((MainActivity) mActivity).prepareFileListView(false);
	}
	
	private void clearCoverCache() {
		File dir = new File(getExternalFilesDir(null), "covers");
		for (File file : dir.listFiles()) {
			if (!file.isDirectory()) {
				file.delete();
			}
		}
	}
	
	public static void clearCache() {
		((MainActivity) mActivity).clearCoverCache();
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
	
		@Override
	public void onNavigationDrawerItemSelected(int position) {
		switch (position) {
			case 0:
				sortMethod = SORT_RECENT;
				prepareFileListView(false);
				break;
			case 1:
				sortMethod = SORT_HOMEBREW;
				prepareFileListView(false);
				break;
			case 2:
				sortMethod = SORT_NONE;
				prepareFileListView(false);
				break;
		}
	}

	@Override
	public void onNavigationDrawerBottomItemSelected(int position) {
		switch (position) {
			case 0:
				displaySettingsActivity();
				break;
			case 1:
				displayAboutDialog();
				break;

		}
	}

	public void restoreActionBar() {
		android.support.v7.app.ActionBar actionBar = getSupportActionBar();
		actionBar.setDisplayShowTitleEnabled(true);
		actionBar.setTitle(R.string.app_name);
		actionBar.setSubtitle(R.string.menu_title_shut);
	}

	
	public boolean onCreateOptionsMenu(Menu menu) {
		if (!mNavigationDrawerFragment.isDrawerOpen()) {
			// Only show items in the action bar relevant to this screen
			// if the drawer is not showing. Otherwise, let the drawer
			// decide what to show in the action bar.
			//getMenuInflater().inflate(R.menu.main, menu);
			restoreActionBar();
			return true;
		}
		return super.onCreateOptionsMenu(menu);
	}

	@Override
	public void onBackPressed() {
		if (mNavigationDrawerFragment.mDrawerLayout != null && mNavigationDrawerFragment.isDrawerOpen()) {
			mNavigationDrawerFragment.mDrawerLayout.closeDrawer(NavigationDrawerFragment.mFragmentContainerView);
			return;
		}
		super.onBackPressed();
		finish();
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
							String serial = gameInfo.getSerial(disk);
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
		
		protected void onPreExecute() {
			progDialog = ProgressDialog.show(MainActivity.this,
				getString(isConfigured ? R.string.updating_db : R.string.search_games),
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
				currentGames = images;
				// Create the list of acceptable images
				populateImages(images);
			} else {
				// Display warning that no disks exist
			}
		}
	}
	
	private View createListItem(final File game, final View childview) {
		if (!isConfigured) {
			
			((TextView) childview.findViewById(R.id.game_text)).setText(game.getName());
			
			childview.findViewById(R.id.childview).setOnClickListener(new OnClickListener() {
				public void onClick(View view) {
					setCurrentDirectory(game.getPath().substring(0,
						game.getPath().lastIndexOf(File.separator)));
					isConfigured = true;
					prepareFileListView(false);
					return;
				}
			});
			
			return childview;
		} else {
			
			((TextView) childview.findViewById(R.id.game_text)).setText(game.getName());
			
			final String[] gameStats = gameInfo.getGameInfo(game, childview);
			
			if (gameStats != null) {
				childview.findViewById(R.id.childview).setOnLongClickListener(
					gameInfo.configureLongClick(gameStats[1], gameStats[2], game));
				
				if (!gameStats[3].equals("404")) {
					gameInfo.getImage(gameStats[0], childview, gameStats[3]);
					((TextView) childview.findViewById(R.id.game_text)).setVisibility(View.GONE);
				}
			} else {
				ImageView preview = (ImageView) childview.findViewById(R.id.game_icon);
				preview.setImageResource(R.drawable.boxart);
				preview.setScaleType(ImageView.ScaleType.FIT_XY);
				((TextView) childview.findViewById(R.id.game_text)).setVisibility(View.VISIBLE);
			}
			
			childview.findViewById(R.id.childview).setOnClickListener(new OnClickListener() {
				public void onClick(View view) {
					launchDisk(game, false);
					return;
				}
			});
			return childview;
		}
	}
	
	private void populateImages(List<File> images) {
		if (sortMethod == SORT_RECENT) {
			@SuppressWarnings("unchecked")
			CompositeFileComparator comparator = new CompositeFileComparator(
				SizeFileComparator.SIZE_REVERSE, LastModifiedFileComparator.LASTMODIFIED_REVERSE);
			comparator.sort(images);
		} else {
			Collections.sort(images);
		}
		GridView gameGrid = (GridView) findViewById(R.id.game_grid);
		if (gameGrid != null && gameGrid.isShown()) {
			gameGrid.setAdapter(null);
		}

		GamesAdapter adapter = new GamesAdapter(MainActivity.this, isConfigured ? R.layout.game_list_item : R.layout.file_list_item, images);
		/*
		gameGrid.setNumColumns(-1);
		-1 = autofit
		or set a number if you like
		 */
		if (isConfigured){
			gameGrid.setColumnWidth((int) getResources().getDimension(R.dimen.cover_width));
		}
		gameGrid.setAdapter(adapter);
		gameGrid.invalidate();

	}
	
	public class GamesAdapter extends ArrayAdapter<File> {

		private final int layoutid;
		private List<File> games;
		
		public GamesAdapter(Context context, int ResourceId, List<File> images) {
			super(context, ResourceId, images);
			this.games = images;
			this.layoutid = ResourceId;
		}

		public int getCount() {
			//return mThumbIds.length;
			return games.size();
		}

		public File getItem(int position) {
			return games.get(position);
		}

		public long getItemId(int position) {
			return position;
		}
		
		@Override
		public View getView(int position, View convertView, ViewGroup parent) {
			View v = convertView;
			if (v == null) {
				LayoutInflater vi = (LayoutInflater) getSystemService(Context.LAYOUT_INFLATER_SERVICE);
				v = vi.inflate(layoutid, null);
			}
			final File game = games.get(position);
			if (game != null) {
				createListItem(game, v);
			}
			return v;
		}
	}
	
	public static void launchGame(File game) {
		((MainActivity) mActivity).launchDisk(game, false);
	}
	
	private void launchDisk (File game, boolean terminate) {
		try
		{
			if(IsLoadableExecutableFileName(game.getPath()))
			{
				game.setLastModified(System.currentTimeMillis());
				NativeInterop.loadElf(game.getPath());
			}
			else
			{
				game.setLastModified(System.currentTimeMillis());
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
		if (terminate) {
			finish();
		}
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

	private void prepareFileListView(boolean retainList)
	{
		if (gameInfo == null) {
			gameInfo = new GameInfo(MainActivity.this);
		}
		
		String sdcard = getCurrentDirectory();
		if (!sdcard.equals(Environment.getExternalStorageDirectory().getAbsolutePath())) {
			isConfigured = true;
		}
		
		if (isConfigured && retainList) {
			populateImages(currentGames);
		} else {
			if (sortMethod == SORT_HOMEBREW) {
				new ImageFinder(R.array.homebrew).execute(sdcard);
			} else {
				new ImageFinder(R.array.disks).execute(sdcard);
			}
		}
	}
}
