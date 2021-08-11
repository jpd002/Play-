package com.virtualapplications.play;

import android.Manifest;
import android.app.*;
import android.app.AlertDialog;
import android.app.ProgressDialog;
import android.content.*;
import android.content.pm.*;
import android.content.res.Configuration;
import android.graphics.Color;
import android.graphics.drawable.GradientDrawable;
import android.net.Uri;
import android.os.*;

import androidx.annotation.NonNull;
import androidx.documentfile.provider.DocumentFile;
import androidx.preference.PreferenceManager;
import androidx.core.app.ActivityCompat;
import androidx.core.content.ContextCompat;
import androidx.appcompat.app.*;
import androidx.appcompat.app.ActionBar;
import androidx.appcompat.widget.Toolbar;

import android.view.*;
import android.view.View;
import android.widget.*;
import android.widget.GridView;

import java.net.URLDecoder;
import java.nio.charset.StandardCharsets;
import java.text.*;
import java.util.*;

import com.virtualapplications.play.database.GameIndexer;
import com.virtualapplications.play.database.GameInfo;

import static com.virtualapplications.play.BootablesInterop.PurgeInexistingFiles;
import static com.virtualapplications.play.BootablesInterop.UnregisterBootable;
import static com.virtualapplications.play.BootablesInterop.getBootables;
import static com.virtualapplications.play.BootablesInterop.setLastBootedTime;
import static com.virtualapplications.play.BootablesInterop.SORT_RECENT;
import static com.virtualapplications.play.BootablesInterop.SORT_HOMEBREW;
import static com.virtualapplications.play.BootablesInterop.SORT_NONE;
import static com.virtualapplications.play.Constants.PREF_UI_CLEAR_UNAVAILABLE;
import static com.virtualapplications.play.Constants.PREF_UI_RESCAN;
import static com.virtualapplications.play.ThemeManager.getThemeColor;

public class MainActivity extends AppCompatActivity implements NavigationDrawerFragment.NavigationDrawerCallbacks, SharedPreferences.OnSharedPreferenceChangeListener
{
	private int currentOrientation;
	private GameInfo gameInfo;
	protected NavigationDrawerFragment mNavigationDrawerFragment;
	private List<Bootable> currentGames = new ArrayList<>();
	private int sortMethod = SORT_NONE;
	private String navSubtitle;
	private Toolbar toolbar;

	static final int g_settingsRequestCode = 0xDEAD;
	static final int g_folderPickerRequestCode = 0xBEEF;

	@Override
	protected void onCreate(Bundle savedInstanceState)
	{
		super.onCreate(savedInstanceState);
		//Log.w(Constants.TAG, "MainActivity - onCreate");

		currentOrientation = getResources().getConfiguration().orientation;

		ThemeManager.applyTheme(this, toolbar);
		if(isAndroidTV(this))
		{
			setContentView(R.layout.tele);
		}
		else
		{
			setContentView(R.layout.main);
		}

//		if (isAndroidTV(this)) {
//			// Load the menus for Android TV
//		} else {
		toolbar = findViewById(R.id.main_toolbar);
		setSupportActionBar(toolbar);
		toolbar.bringToFront();
		setUIcolor();

		mNavigationDrawerFragment = (NavigationDrawerFragment)
				getFragmentManager().findFragmentById(R.id.navigation_drawer);

		// Set up the drawer.
		mNavigationDrawerFragment.setUp(
				R.id.navigation_drawer,
				findViewById(R.id.drawer_layout));

		int attributeResourceId = getThemeColor(this, R.attr.colorPrimaryDark);
		findViewById(R.id.navigation_drawer).setBackgroundColor(Color.parseColor(
				("#" + Integer.toHexString(attributeResourceId)).replace("#ff", "#8e")
		));
//		}

		if(ContextCompat.checkSelfPermission(this, Manifest.permission.WRITE_EXTERNAL_STORAGE) != PackageManager.PERMISSION_GRANTED)
		{
			ActivityCompat.requestPermissions(this, new String[]{Manifest.permission.WRITE_EXTERNAL_STORAGE}, Constants.READ_WRITE_PERMISSION);
		}
		else
		{
			Startup();
		}
	}

	@Override
	public void onDestroy()
	{
		SharedPreferences prefs = PreferenceManager.getDefaultSharedPreferences(this);
		prefs.unregisterOnSharedPreferenceChangeListener(this);
		super.onDestroy();
	}

	private void Startup()
	{
		NativeInterop.setFilesDirPath(getApplicationContext().getFilesDir().getAbsolutePath());
		NativeInterop.setCacheDirPath(getApplicationContext().getCacheDir().getAbsolutePath());
		NativeInterop.setAssetManager(getAssets());
		NativeInterop.setContentResolver(getApplicationContext().getContentResolver());

		EmulatorActivity.RegisterPreferences();

		if(!NativeInterop.isVirtualMachineCreated())
		{
			NativeInterop.createVirtualMachine();
		}

		gameInfo = new GameInfo(MainActivity.this);

		SharedPreferences sp = PreferenceManager.getDefaultSharedPreferences(this);
		sortMethod = sp.getInt("sortMethod", SORT_NONE);
		onNavigationDrawerItemSelected(sortMethod);
		sp.registerOnSharedPreferenceChangeListener(this);
	}

	@Override
	public void onRequestPermissionsResult(int requestCode, @NonNull String[] permissions, @NonNull int[] grantResults)
	{
		if(requestCode == Constants.READ_WRITE_PERMISSION)
		{// If request is cancelled, the result arrays are empty.
			if(grantResults.length > 0 && grantResults[0] == PackageManager.PERMISSION_GRANTED)
			{
				Startup();
			}
			else
			{
				new AlertDialog.Builder(this)
						.setTitle(R.string.permission_dialog_title)
						.setMessage(R.string.permission_dialog_summary)
						.setPositiveButton(android.R.string.ok, (dialog, id) ->
								finish()
						)
						.create()
						.show();
			}
		}
	}

	private void setUIcolor()
	{
		View content = findViewById(R.id.content_frame);
		if(content != null)
		{
			int[] colors = new int[2];// you can increase array size to add more colors to gradient.
			int topgradientcolor = getThemeColor(this, R.attr.colorPrimary);

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
		int attributeResourceId = getThemeColor(this, R.attr.colorPrimaryDark);
		findViewById(R.id.navigation_drawer).setBackgroundColor(Color.parseColor(
				("#" + Integer.toHexString(attributeResourceId)).replace("#ff", "#8e")
		));
	}

	private void displaySimpleMessage(String title, String message)
	{
		new AlertDialog.Builder(this)
				.setTitle(title)
				.setMessage(message)
				.setPositiveButton(android.R.string.ok, null)
				.create()
				.show();
	}

	private void displayGameNotFound(final Bootable game)
	{
		new AlertDialog.Builder(this)
				.setTitle(R.string.not_found)
				.setMessage(R.string.game_unavailable)
				.setPositiveButton(android.R.string.ok, (dialog, id) -> {
					UnregisterBootable(game.path);
					prepareFileListView(false);
				})
				.setNegativeButton(android.R.string.cancel, null)
				.create()
				.show();
	}

	private void displaySettingsActivity()
	{
		Intent intent = new Intent(getApplicationContext(), SettingsActivity.class);
		startActivityForResult(intent, g_settingsRequestCode);
	}

	private void displayFolderPicker()
	{
		Intent intent = new Intent(Intent.ACTION_OPEN_DOCUMENT_TREE);
		startActivityForResult(intent, g_folderPickerRequestCode);
	}

	protected void onActivityResult(int requestCode, int resultCode, Intent data)
	{
		super.onActivityResult(requestCode, resultCode, data);
		if(requestCode == g_settingsRequestCode)
		{
			ThemeManager.applyTheme(this, toolbar);
			setUIcolor();
		}

		if(requestCode == 1 && resultCode == RESULT_OK)
		{
			if(data != null)
			{
				gameInfo.removeBitmapFromMemCache(data.getStringExtra("indexid"));
			}
			prepareFileListView(false);
		}

		if(requestCode == g_folderPickerRequestCode && resultCode == RESULT_OK)
		{
			Uri folderUri = data.getData();
			getContentResolver().takePersistableUriPermission(folderUri, Intent.FLAG_GRANT_READ_URI_PERMISSION);
			processFolder(data.getData());
		}
	}

	private void displayAboutDialog()
	{
		Date buildDate = BuildConfig.buildTime;
		String buildDateString = new SimpleDateFormat("yyyy/MM/dd K:mm a", Locale.getDefault()).format(buildDate);
		String timestamp = buildDateString.substring(11).startsWith("0:")
				? buildDateString.replace("0:", "12:") : buildDateString;
		String aboutMessage = String.format("Version: %s Date: %s", BuildConfig.VERSION_NAME, timestamp);
		displaySimpleMessage("About Play!", aboutMessage);
	}

	@Override
	public void onConfigurationChanged(@NonNull Configuration newConfig)
	{
		super.onConfigurationChanged(newConfig);
		mNavigationDrawerFragment.onConfigurationChanged(newConfig);
		if(newConfig.orientation != currentOrientation)
		{
			currentOrientation = newConfig.orientation;
			setUIcolor();
			prepareFileListView(currentGames != null && !currentGames.isEmpty());
		}
	}

	@Override
	public void onNavigationDrawerItemSelected(int position)
	{
		switch(position)
		{
		case SORT_RECENT:
			sortMethod = SORT_RECENT;
			navSubtitle = getString(R.string.file_list_recent);
			break;
		case SORT_HOMEBREW:
			sortMethod = SORT_HOMEBREW;
			navSubtitle = getString(R.string.file_list_homebrew);
			break;
		case SORT_NONE:
		default:
			sortMethod = SORT_NONE;
			navSubtitle = getString(R.string.file_list_default);
			break;
		}

		ActionBar actionbar = getSupportActionBar();
		if(actionbar != null)
		{
			actionbar.setSubtitle("Games - " + navSubtitle);
		}

		prepareFileListView(false);

		SharedPreferences sp = PreferenceManager.getDefaultSharedPreferences(this);
		sp.edit().putInt("sortMethod", sortMethod).apply();
	}

	@Override
	public void onNavigationDrawerBottomItemSelected(int position)
	{
		switch(position)
		{
		case 0:
			displaySettingsActivity();
			break;
		case 1:
			displayFolderPicker();
			break;
		case 2:
			displayAboutDialog();
			break;
		}
	}

	public void restoreActionBar()
	{
		ActionBar actionBar = getSupportActionBar();
		actionBar.setDisplayShowTitleEnabled(true);
		actionBar.setTitle(R.string.app_name);
		actionBar.setSubtitle("Games - " + navSubtitle);
	}

	public boolean onCreateOptionsMenu(Menu menu)
	{
		if(!mNavigationDrawerFragment.isDrawerOpen())
		{
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
	public void onBackPressed()
	{
		if(NavigationDrawerFragment.mDrawerLayout != null && mNavigationDrawerFragment.isDrawerOpen())
		{
			NavigationDrawerFragment.mDrawerLayout.closeDrawer(NavigationDrawerFragment.mFragmentContainerView);
			return;
		}
		super.onBackPressed();
		finish();
	}

	@Override
	public void onSharedPreferenceChanged(SharedPreferences sharedPreferences, String key)
	{
		if(key.equals(PREF_UI_RESCAN))
		{
			if(sharedPreferences.getBoolean(PREF_UI_RESCAN, false))
			{
				sharedPreferences.edit().putBoolean(PREF_UI_RESCAN, false).apply();
				prepareFileListView(false, true);
			}
		}
		else if(key.equals(PREF_UI_CLEAR_UNAVAILABLE))
		{
			if(sharedPreferences.getBoolean(PREF_UI_CLEAR_UNAVAILABLE, false))
			{
				sharedPreferences.edit().putBoolean(PREF_UI_CLEAR_UNAVAILABLE, false).apply();

				PurgeInexistingFiles();

				prepareFileListView(false);
			}
		}
	}

	private void processFolder(Uri folderUri)
	{
		ContentResolver res = getApplicationContext().getContentResolver();
		DocumentFile folderDoc = DocumentFile.fromTreeUri(this, folderUri);
		for(DocumentFile fileDoc : folderDoc.listFiles())
		{
			try
			{
				String docUriString = fileDoc.getUri().toString();
				String decodedUriString = URLDecoder.decode(docUriString, StandardCharsets.UTF_8.name());
				BootablesInterop.tryRegisterBootable(decodedUriString);
			}
			catch(Exception ex)
			{

			}
		}
		prepareFileListView(false);
	}

	private final class ImageFinder extends AsyncTask<String, Integer, List<Bootable>>
	{
		private final boolean fullscan;
		private ProgressDialog progDialog;

		public ImageFinder(boolean fullscan)
		{
			this.fullscan = fullscan;
		}

		protected void onPreExecute()
		{
			progDialog = ProgressDialog.show(MainActivity.this,
					getString(R.string.search_games),
					getString(R.string.search_games_msg), true);
		}

		@Override
		protected List<Bootable> doInBackground(String... paths)
		{
			if(fullscan)
			{
				GameIndexer.fullScan();
			}
			else
			{
				GameIndexer.startupScan();
			}

			return new ArrayList<>(Arrays.asList(getBootables(sortMethod)));
		}

		@Override
		protected void onPostExecute(List<Bootable> images)
		{
			if(progDialog != null && progDialog.isShowing())
			{
				progDialog.dismiss();
			}
			currentGames = images;
			// Create the list of acceptable images
			populateImages(images);
		}
	}

	private void populateImages(List<Bootable> images)
	{
		GridView gameGrid = findViewById(R.id.game_grid);
		if(gameGrid != null)
		{
			gameGrid.setAdapter(null);
			if(isAndroidTV(this))
			{
				gameGrid.setOnItemClickListener((parent, v, position, id) -> v.performClick());
			}
			if(images == null || images.isEmpty())
			{
				// Display warning that no disks exist
				ArrayAdapter<String> adapter = new ArrayAdapter<>(this, android.R.layout.simple_list_item_1, (sortMethod == SORT_RECENT) ? new String[]{getString(R.string.no_recent_adapter)} : new String[]{getString(R.string.no_game_found_adapter)});
				gameGrid.setNumColumns(1);
				gameGrid.setAdapter(adapter);
			}
			else
			{
				GamesAdapter adapter = new GamesAdapter(this, R.layout.game_list_item, images, gameInfo);
				/*
				-1 = autofit
				 */
				gameGrid.setNumColumns(-1);
				gameGrid.setColumnWidth((int)getResources().getDimension(R.dimen.cover_width));

				gameGrid.setAdapter(adapter);
				gameGrid.invalidate();
			}
		}
	}

	public void launchGame(Bootable game)
	{
		if(BootablesInterop.DoesBootableExist(game.path))
		{
			setLastBootedTime(game.path, System.currentTimeMillis());
			try
			{
				VirtualMachineManager.launchGame(this, game.path);
			}
			catch(Exception e)
			{
				displaySimpleMessage("Error", e.getMessage());
			}
		}
		else
		{
			displayGameNotFound(game);
		}
	}

	private boolean isAndroidTV(Context context)
	{
		UiModeManager uiModeManager = (UiModeManager)
				context.getSystemService(Context.UI_MODE_SERVICE);
		return uiModeManager.getCurrentModeType() == Configuration.UI_MODE_TYPE_TELEVISION;
	}

	public void prepareFileListView(boolean retainList)
	{
		prepareFileListView(retainList, false);
	}

	private void prepareFileListView(boolean retainList, boolean fullscan)
	{
		if(gameInfo == null)
		{
			gameInfo = new GameInfo(MainActivity.this);
		}

		if(retainList)
		{
			populateImages(currentGames);
		}
		else
		{
			new ImageFinder(fullscan).execute();
		}
	}
}
