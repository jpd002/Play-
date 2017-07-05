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
import android.os.*;
import android.preference.PreferenceManager;
import android.support.v4.app.ActivityCompat;
import android.support.v4.content.ContextCompat;
import android.support.v7.app.*;
import android.support.v7.app.ActionBar;
import android.support.v7.widget.Toolbar;
import android.util.Log;
import android.view.*;
import android.view.View;
import android.widget.*;
import android.widget.AdapterView.OnItemClickListener;
import android.widget.GridView;
import android.support.v4.widget.DrawerLayout;

import java.io.File;
import java.text.*;
import java.util.*;

import com.virtualapplications.play.database.GameIndexer;
import com.virtualapplications.play.database.GameInfo;
import com.virtualapplications.play.database.IndexingDB;
import com.virtualapplications.play.database.SqliteHelper.Games;

import static com.virtualapplications.play.ThemeManager.getThemeColor;

public class MainActivity extends AppCompatActivity implements NavigationDrawerFragment.NavigationDrawerCallbacks, SharedPreferences.OnSharedPreferenceChangeListener
{
	static Activity mActivity;
	private int currentOrientation;
	private GameInfo gameInfo;
	protected NavigationDrawerFragment mNavigationDrawerFragment;
	private List<GameInfoStruct> currentGames = new ArrayList<>();
	public static final int SORT_RECENT = 0;
	public static final int SORT_HOMEBREW = 1;
	public static final int SORT_NONE = 2;
	private int sortMethod = SORT_NONE;
	private String navSubtitle;

	@Override
	protected void onCreate(Bundle savedInstanceState)
	{
		super.onCreate(savedInstanceState);
		//Log.w(Constants.TAG, "MainActivity - onCreate");

		currentOrientation = getResources().getConfiguration().orientation;
		mActivity = MainActivity.this;

		ThemeManager.applyTheme(this);
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
		Toolbar toolbar = (Toolbar)findViewById(R.id.my_awesome_toolbar);
		setSupportActionBar(toolbar);
		toolbar.bringToFront();
		setUIcolor();

		mNavigationDrawerFragment = (NavigationDrawerFragment)
				getFragmentManager().findFragmentById(R.id.navigation_drawer);

		// Set up the drawer.
		mNavigationDrawerFragment.setUp(
				R.id.navigation_drawer,
				(DrawerLayout)findViewById(R.id.drawer_layout));

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
		NativeInterop.setFilesDirPath(Environment.getExternalStorageDirectory().getAbsolutePath());
		NativeInterop.setAssetManager(getAssets());

		EmulatorActivity.RegisterPreferences();

		if(!NativeInterop.isVirtualMachineCreated())
		{
			NativeInterop.createVirtualMachine();
		}

		gameInfo = new GameInfo(MainActivity.this);
		getContentResolver().call(Games.GAMES_URI, "importDb", null, null);

		SharedPreferences sp = PreferenceManager.getDefaultSharedPreferences(this);
		sortMethod = sp.getInt("sortMethod", SORT_NONE);
		onNavigationDrawerItemSelected(sortMethod);
		sp.registerOnSharedPreferenceChangeListener(this);
	}

	@Override
	public void onRequestPermissionsResult(int requestCode, String permissions[], int[] grantResults)
	{
		switch(requestCode)
		{
		case Constants.READ_WRITE_PERMISSION:
		{
			// If request is cancelled, the result arrays are empty.
			if(grantResults.length > 0 && grantResults[0] == PackageManager.PERMISSION_GRANTED)
			{
				Startup();
			}
			else
			{
				AlertDialog.Builder builder = new AlertDialog.Builder(this);

				builder.setTitle("Permission Request");
				builder.setMessage("Please Grant Permission,\nWithout Read/Write Permission, PLAY! wouldn't be able to find your games or save your progress.");

				builder.setPositiveButton("OK",
						new DialogInterface.OnClickListener()
						{
							@Override
							public void onClick(DialogInterface dialog, int id)
							{
								finish();
							}
						}
				);

				AlertDialog dialog = builder.create();
				dialog.show();
			}
			return;
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

	private void displayGameNotFound(final GameInfoStruct game)
	{
		AlertDialog.Builder builder = new AlertDialog.Builder(this);

		builder.setTitle(R.string.not_found);
		builder.setMessage(R.string.game_unavailable);

		builder.setPositiveButton("Yes",
				new DialogInterface.OnClickListener()
				{
					@Override
					public void onClick(DialogInterface dialog, int id)
					{
						game.removeIndex(MainActivity.this);
						prepareFileListView(false);
					}
				}
		);
		builder.setNegativeButton("No",
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

	protected void onActivityResult(int requestCode, int resultCode, Intent data)
	{
		if(requestCode == 0)
		{
			ThemeManager.applyTheme(this);
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
	}

	private void displayAboutDialog()
	{
		Date buildDate = BuildConfig.buildTime;
		String buildDateString = new SimpleDateFormat("yyyy/MM/dd K:mm a", Locale.getDefault()).format(buildDate);
		String timestamp = buildDateString.substring(11, buildDateString.length()).startsWith("0:")
				? buildDateString.replace("0:", "12:") : buildDateString;
		String aboutMessage = String.format("Build Date: %s", timestamp);
		displaySimpleMessage("About Play!", aboutMessage);
	}

	@Override
	public void onConfigurationChanged(Configuration newConfig)
	{
		super.onConfigurationChanged(newConfig);
		mNavigationDrawerFragment.onConfigurationChanged(newConfig);
		if(newConfig.orientation != currentOrientation)
		{
			currentOrientation = newConfig.orientation;
			setUIcolor();
			if(currentGames != null && !currentGames.isEmpty())
			{
				prepareFileListView(true);
			}
			else
			{
				prepareFileListView(false);
			}
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
			displayAboutDialog();
			break;
		}
	}

	public void restoreActionBar()
	{
		android.support.v7.app.ActionBar actionBar = getSupportActionBar();
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
		if(mNavigationDrawerFragment.mDrawerLayout != null && mNavigationDrawerFragment.isDrawerOpen())
		{
			mNavigationDrawerFragment.mDrawerLayout.closeDrawer(NavigationDrawerFragment.mFragmentContainerView);
			return;
		}
		super.onBackPressed();
		finish();
	}

	@Override
	public void onSharedPreferenceChanged(SharedPreferences sharedPreferences, String key)
	{
		if(key.equals(SettingsActivity.RESCAN))
		{
			if(sharedPreferences.getBoolean(SettingsActivity.RESCAN, false))
			{
				sharedPreferences.edit().putBoolean(SettingsActivity.RESCAN, false).apply();
				prepareFileListView(false, true);
			}
		}
		else if(key.equals(SettingsActivity.CLEAR_UNAVAILABLE))
		{
			if(sharedPreferences.getBoolean(SettingsActivity.CLEAR_UNAVAILABLE, false))
			{
				sharedPreferences.edit().putBoolean(SettingsActivity.CLEAR_UNAVAILABLE, false).apply();
				IndexingDB iDB = new IndexingDB(this);
				iDB.RemoveUnavailable();
				iDB.close();
				prepareFileListView(false);
			}
		}
	}

	private final class ImageFinder extends AsyncTask<String, Integer, List<GameInfoStruct>>
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
		protected List<GameInfoStruct> doInBackground(String... paths)
		{

			GameIndexer GI = new GameIndexer(MainActivity.this);
			if(fullscan)
			{
				GI.fullScan();
			}
			else
			{
				GI.startupScan();
			}
			return GI.getIndexGISList(sortMethod);
		}

		@Override
		protected void onPostExecute(List<GameInfoStruct> images)
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

	private void populateImages(List<GameInfoStruct> images)
	{
		GridView gameGrid = (GridView)findViewById(R.id.game_grid);
		if(gameGrid != null)
		{
			gameGrid.setAdapter(null);
			if(isAndroidTV(this))
			{
				gameGrid.setOnItemClickListener(new OnItemClickListener()
				{
					public void onItemClick(AdapterView<?> parent, View v, int position, long id)
					{
						v.performClick();
					}
				});
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

	public void launchGame(GameInfoStruct game)
	{
		if(game.getFile().exists())
		{
			game.setlastplayed(this);
			try
			{
				VirtualMachineManager.launchDisk(this, game.getFile());
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
		if(Build.VERSION.SDK_INT >= Build.VERSION_CODES.ICE_CREAM_SANDWICH)
		{
			UiModeManager uiModeManager = (UiModeManager)
					context.getSystemService(Context.UI_MODE_SERVICE);
			if(uiModeManager.getCurrentModeType() == Configuration.UI_MODE_TYPE_TELEVISION)
			{
				return true;
			}
		}
		return false;
	}

	public void prepareFileListView(boolean retainList)
	{
		((MainActivity)mActivity).prepareFileListView(retainList, false);
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
