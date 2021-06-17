package com.virtualapplications.play;

import android.app.Fragment;
import android.graphics.Color;
import android.os.Bundle;

import androidx.core.view.GravityCompat;
import androidx.drawerlayout.widget.DrawerLayout;
import android.view.LayoutInflater;
import android.view.View;
import android.view.ViewGroup;
import android.widget.AdapterView;
import android.widget.ArrayAdapter;
import android.widget.LinearLayout;
import android.widget.ListView;

public class EmulatorDrawerFragment extends Fragment
{
	DrawerLayout _drawerLayout;
	View _fragmentView;
	ListView _listView;
	EventListener _eventListener;

	public interface EventListener
	{
		void onExitSelected();

		void onSaveStateSelected();

		void onLoadStateSelected();

		void onSettingsSelected();
	}

	public EmulatorDrawerFragment()
	{

	}

	public void setEventListener(EventListener eventListener)
	{
		_eventListener = eventListener;
	}

	@Override
	public View onCreateView(LayoutInflater inflater, ViewGroup container, Bundle savedInstanceState)
	{
		LinearLayout layout = (LinearLayout)inflater.inflate(R.layout.fragment_emulator_drawer, container, false);
		_listView = (ListView)layout.findViewById(R.id.fragment_emulator_drawer_list);
		return layout;
	}

	@Override
	public void onActivityCreated(Bundle savedInstanceState)
	{
		super.onActivityCreated(savedInstanceState);

		_listView.setAdapter(
				new ArrayAdapter<>(
						getActivity(),
						android.R.layout.simple_list_item_activated_1,
						android.R.id.text1,
						new String[]
								{
										getString(R.string.emulator_drawer_savestate),
										getString(R.string.emulator_drawer_loadstate),
										getString(R.string.main_menu_settings),
										getString(R.string.emulator_drawer_exit),
								}
				)
		);
		_listView.setOnItemClickListener(
				new AdapterView.OnItemClickListener()
				{
					@Override
					public void onItemClick(AdapterView<?> parent, View view, int position, long id)
					{
						selectItem(position);
					}
				}
		);
	}

	public void setUp(View fragmentView, DrawerLayout drawerLayout)
	{
		_fragmentView = fragmentView;
		_drawerLayout = drawerLayout;
		_drawerLayout.setDrawerShadow(R.drawable.drawer_shadow, GravityCompat.START);

		int color = ThemeManager.getThemeColor(getActivity(), R.attr.colorPrimaryDark);
		_fragmentView.setBackgroundColor(
				Color.parseColor(("#" + Integer.toHexString(color)).replace("#ff", "#8e"))
		);
	}

	public void openDrawer()
	{
		_drawerLayout.openDrawer(_fragmentView);
	}

	public void closeDrawer()
	{
		_drawerLayout.closeDrawer(_fragmentView);
	}

	public boolean isDrawerOpened()
	{
		return (_drawerLayout != null) && (_drawerLayout.isDrawerOpen(_fragmentView));
	}

	private void selectItem(int position)
	{
		if(_eventListener == null) return;
		switch(position)
		{
		case 0:
			_eventListener.onSaveStateSelected();
			break;
		case 1:
			_eventListener.onLoadStateSelected();
			break;
		case 2:
			_eventListener.onSettingsSelected();
			break;
		case 3:
			_eventListener.onExitSelected();
			break;
		}
	}
}
