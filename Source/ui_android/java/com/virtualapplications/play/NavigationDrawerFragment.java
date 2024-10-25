package com.virtualapplications.play;

import android.app.Activity;
import android.app.Fragment;
import android.content.SharedPreferences;
import android.content.res.Configuration;
import android.graphics.Color;
import android.os.Bundle;
import androidx.preference.PreferenceManager;
import androidx.core.view.GravityCompat;
import androidx.drawerlayout.widget.DrawerLayout;
import androidx.appcompat.app.ActionBar;
import androidx.appcompat.app.ActionBarDrawerToggle;
import androidx.appcompat.app.AppCompatActivity;
import android.view.LayoutInflater;
import android.view.Menu;
import android.view.MenuInflater;
import android.view.MenuItem;
import android.view.View;
import android.view.ViewGroup;
import android.widget.ArrayAdapter;
import android.widget.LinearLayout;
import android.widget.ListView;

import static com.virtualapplications.play.ThemeManager.getThemeColor;

/**
 * Fragment used for managing interactions for and presentation of a navigation drawer.
 * See the <a href="https://developer.android.com/design/patterns/navigation-drawer.html#Interaction">
 * design guidelines</a> for a complete explanation of the behaviors implemented here.
 */
public class NavigationDrawerFragment extends Fragment
{
	/**
	 * Remember the position of the selected item.
	 */
	private static final String STATE_SELECTED_POSITION = "selected_navigation_drawer_position";
	/**
	 * Per the design guidelines, you should show the drawer on launch until the user manually
	 * expands it. This shared preference tracks this.
	 */
	private static final String PREF_USER_LEARNED_DRAWER = "navigation_drawer_learned";
	/**
	 * A pointer to the current callbacks instance (the Activity).
	 */
	private NavigationDrawerCallbacks mCallbacks;
	/**
	 * Helper component that ties the action bar to the navigation drawer.
	 */
	private ActionBarDrawerToggle mDrawerToggle;
	protected static DrawerLayout mDrawerLayout;
	private ListView mDrawerListView;
	protected static View mFragmentContainerView;
	private int mCurrentSelectedPosition = 0;
	private boolean mFromSavedInstanceState;
	private boolean mUserLearnedDrawer;
	private ListView mDrawerListView_bottom;

	@Override
	public void onCreate(Bundle savedInstanceState)
	{
		super.onCreate(savedInstanceState);

		// Read in the flag indicating whether or not the user has demonstrated awareness of the
		// drawer. See PREF_USER_LEARNED_DRAWER for details.
		SharedPreferences sp = PreferenceManager.getDefaultSharedPreferences(getActivity());
		mUserLearnedDrawer = sp.getBoolean(PREF_USER_LEARNED_DRAWER, false);

		if(savedInstanceState != null)
		{
			mCurrentSelectedPosition = savedInstanceState.getInt(STATE_SELECTED_POSITION);
			mFromSavedInstanceState = true;
		}
		else
		{
			// Select either the default item (0) or the last selected item.
			mCurrentSelectedPosition = sp.getInt("sortMethod.v2", BootablesInterop.SORT_NONE);
		}
	}

	@Override
	public void onActivityCreated(Bundle savedInstanceState)
	{
		super.onActivityCreated(savedInstanceState);
		// Indicate that this fragment would like to influence the set of actions in the action bar.
		setHasOptionsMenu(true);

		mDrawerListView.setAdapter(new ArrayAdapter<>(
				((AppCompatActivity)getActivity()).getSupportActionBar().getThemedContext(),
				android.R.layout.simple_list_item_activated_1,
				android.R.id.text1,
				new String[]{
						getString(R.string.file_list_recent),
						getString(R.string.file_list_default),
						getString(R.string.file_list_homebrew),
				}));
		//Thunder07: Using 'post' might seem weird, but thats the only way I found for setting the selection
		//highlight on startup, every other way returns null.
		mDrawerListView.post(() -> {
			int attributeResourceId = getThemeColor(getActivity(), R.attr.colorPrimaryDark);
			View childView = mDrawerListView.getChildAt(mCurrentSelectedPosition);
			if(childView != null)
			{
				childView.setBackgroundColor(attributeResourceId);
			}
		});

		mDrawerListView_bottom.setAdapter(new ArrayAdapter<>(
				((AppCompatActivity)getActivity()).getSupportActionBar().getThemedContext(),
				android.R.layout.simple_list_item_activated_1,
				android.R.id.text1,
				new String[]{
						getString(R.string.main_menu_settings),
						getString(R.string.main_menu_addfolder),
						getString(R.string.main_menu_about),
				}));
	}

	@Override
	public View onCreateView(LayoutInflater inflater, ViewGroup container,
							 Bundle savedInstanceState)
	{

		LinearLayout mDrawerRelativeLayout = (LinearLayout)inflater.inflate(
				R.layout.fragment_navigation_drawer, container, false);
		mDrawerListView = mDrawerRelativeLayout.findViewById(R.id.nav_listview);
		mDrawerListView.setOnItemClickListener((parent, view, position, id) ->
				selectItem(position));

		mDrawerListView_bottom = mDrawerRelativeLayout.findViewById(R.id.nav_listview_bottom);
		mDrawerListView_bottom.setOnItemClickListener((parent, view, position, id) ->
				selectBottomItem(position));
		return mDrawerRelativeLayout;
	}

	public boolean isDrawerOpen()
	{
		return mDrawerLayout != null && mDrawerLayout.isDrawerOpen(mFragmentContainerView);
	}

	/**
	 * Users of this fragment must call this method to set up the navigation drawer interactions.
	 *
	 * @param fragmentId   The android:id of this fragment in its activity's layout.
	 * @param drawerLayout The DrawerLayout containing this fragment's UI.
	 */
	public void setUp(int fragmentId, DrawerLayout drawerLayout)
	{
		mFragmentContainerView = getActivity().findViewById(fragmentId);
		mDrawerLayout = drawerLayout;
		// set a custom shadow that overlays the main content when the drawer opens
		mDrawerLayout.setDrawerShadow(R.drawable.drawer_shadow, GravityCompat.START);
		// set up the drawer's list view with items and click listener

		ActionBar actionBar = ((AppCompatActivity)getActivity()).getSupportActionBar();
		actionBar.setDisplayHomeAsUpEnabled(true);
		actionBar.setHomeButtonEnabled(true);

		// to set color
		// ActionBarDrawerToggle ties together the the proper interactions
		// between the navigation drawer and the action bar app icon.

		mDrawerToggle = new ActionBarDrawerToggle(
				getActivity(),                    /* host Activity */
				mDrawerLayout,                    /* DrawerLayout object */
				R.string.navigation_drawer_open,  /* "open drawer" description for accessibility */
				R.string.navigation_drawer_close  /* "close drawer" description for accessibility */
		)
		{
			@Override
			public void onDrawerClosed(View drawerView)
			{
				super.onDrawerClosed(drawerView);
				if(!isAdded())
				{
					return;
				}

				getActivity().invalidateOptionsMenu(); // calls onPrepareOptionsMenu()
			}

			@Override
			public void onDrawerOpened(View drawerView)
			{
				super.onDrawerOpened(drawerView);
				if(!isAdded())
				{
					return;
				}

				if(!mUserLearnedDrawer)
				{
					// The user manually opened the drawer; store this flag to prevent auto-showing
					// the navigation drawer automatically in the future.
					mUserLearnedDrawer = true;
					SharedPreferences sp = PreferenceManager
							.getDefaultSharedPreferences(getActivity());
					sp.edit().putBoolean(PREF_USER_LEARNED_DRAWER, true).apply();
				}

				getActivity().invalidateOptionsMenu(); // calls onPrepareOptionsMenu()
			}
		};
		// If the user hasn't 'learned' about the drawer, open it to introduce them to the drawer,
		// per the navigation drawer design guidelines.
		if(!mUserLearnedDrawer && !mFromSavedInstanceState)
		{
			mDrawerLayout.openDrawer(mFragmentContainerView);
		}
		mDrawerLayout.setDrawerListener(mDrawerToggle);

		// Defer code dependent on restoration of previous instance state.
		mDrawerLayout.post(() -> mDrawerToggle.syncState());
	}

	private void selectItem(int position)
	{
		mCurrentSelectedPosition = position;
		if(mDrawerListView != null)
		{
			mDrawerListView.setItemChecked(position, true);
			for(int i = 0; i < mDrawerListView.getChildCount(); i++)
			{
				if(position == i)
				{
					int attributeResourceId = getThemeColor(getActivity(), R.attr.colorPrimaryDark);
					mDrawerListView.getChildAt(i).setBackgroundColor(attributeResourceId);
				}
				else
				{
					mDrawerListView.getChildAt(i).setBackgroundColor(Color.TRANSPARENT);
				}
			}
		}
		if(mDrawerLayout != null)
		{
			mDrawerLayout.closeDrawer(mFragmentContainerView);
		}
		if(mCallbacks != null)
		{
			mCallbacks.onNavigationDrawerItemSelected(position);
		}
	}

	private void selectBottomItem(int position)
	{
		if(mDrawerListView != null)
		{
			mDrawerListView.setItemChecked(position, true);
		}
		if(mDrawerLayout != null)
		{
			mDrawerLayout.closeDrawer(mFragmentContainerView);
		}
		if(mCallbacks != null)
		{
			mCallbacks.onNavigationDrawerBottomItemSelected(position);
		}
	}

	@Override
	public void onAttach(Activity activity)
	{
		super.onAttach(activity);
		try
		{
			mCallbacks = (NavigationDrawerCallbacks)activity;
		}
		catch(ClassCastException e)
		{
			throw new ClassCastException("Activity must implement NavigationDrawerCallbacks.");
		}
	}

	@Override
	public void onDetach()
	{
		super.onDetach();
		mCallbacks = null;
	}

	@Override
	public void onSaveInstanceState(Bundle outState)
	{
		super.onSaveInstanceState(outState);

		outState.putInt(STATE_SELECTED_POSITION, mCurrentSelectedPosition);
	}

	@Override
	public void onConfigurationChanged(Configuration newConfig)
	{
		super.onConfigurationChanged(newConfig);
		// Forward the new configuration the drawer toggle component.
		mDrawerToggle.onConfigurationChanged(newConfig);
	}

	@Override
	public void onCreateOptionsMenu(Menu menu, MenuInflater inflater)
	{
		// If the drawer is open, show the global app actions in the action bar. See also
		// showGlobalContextActionBar, which controls the top-left area of the action bar.
		if(mDrawerLayout != null && isDrawerOpen())
		{
			//inflater.inflate(R.menu.global, menu);
			showGlobalContextActionBar();
		}
		super.onCreateOptionsMenu(menu, inflater);
	}

	@Override
	public boolean onOptionsItemSelected(MenuItem item)
	{
		if(mDrawerToggle.onOptionsItemSelected(item))
		{
			return true;
		}

		return super.onOptionsItemSelected(item);
	}

	@Override
	public void onResume()
	{
		super.onResume();
		if(mDrawerListView != null)
		{
			int attributeResourceId = getThemeColor(getActivity(), R.attr.colorPrimaryDark);
			View v = mDrawerListView.getChildAt(mCurrentSelectedPosition);
			if(v != null)
			{
				v.setBackgroundColor(attributeResourceId);
			}
		}
	}

	/**
	 * Per the navigation drawer design guidelines, updates the action bar to show the global app
	 * 'context', rather than just what's in the current screen.
	 */
	private void showGlobalContextActionBar()
	{
		ActionBar actionBar = ((AppCompatActivity)getActivity()).getSupportActionBar();
		actionBar.setDisplayShowTitleEnabled(true);
		actionBar.setNavigationMode(ActionBar.NAVIGATION_MODE_STANDARD);
		actionBar.setSubtitle(getString(R.string.menu_title_open));
	}

	/**
	 * Callbacks interface that all activities using this fragment must implement.
	 */
	public interface NavigationDrawerCallbacks
	{
		/**
		 * Called when an item in the navigation drawer is selected.
		 */
		void onNavigationDrawerItemSelected(int position);

		void onNavigationDrawerBottomItemSelected(int position);
	}
}
