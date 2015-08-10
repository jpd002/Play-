package com.virtualapplications.play;

import android.app.Activity;
import android.os.*;
import android.preference.*;
import android.view.LayoutInflater;
import android.view.View;
import android.widget.*;
import java.util.*;
import android.support.v7.widget.Toolbar;
import android.graphics.Point;

public class SettingsActivity extends PreferenceActivity
{
	@Override
	public void onDestroy()
	{
		super.onDestroy();
		SettingsManager.save();
	}

	@Override
	public void onBuildHeaders(List<Header> target)
	{

		loadHeadersFromResource(R.xml.settings_headers, target);
	}

	@Override
	protected void onPostCreate(Bundle savedInstanceState) {
		super.onPostCreate(savedInstanceState);
		LinearLayout root =  (LinearLayout)findViewById(android.R.id.list).getParent().getParent().getParent();
		Toolbar bar = (Toolbar)LayoutInflater.from(this).inflate(R.layout.settings_toolbar, null, false);
		Point p = MainActivity.getNavigationBarSize(this);
		if (p != null){
			/*
			This will take account of nav bar to right
			Not sure if there is a way to detect left thus always pad right for now
			*/
			if (p.x != 0){
				root.setPadding(
						root.getPaddingLeft(), 
						root.getPaddingTop(), 
						root.getPaddingRight() + p.x, 
						root.getPaddingBottom());
			}
		}
		bar.setNavigationOnClickListener(new View.OnClickListener() {
			@Override
			public void onClick(View v) {
				onBackPressed();
			}
		});
		root.addView(bar, 0);
	}

	@Override
	protected void onResume(){
		super.onResume();
		ChangeTheme(null, this);
	}

	public static void ChangeTheme(Object pos, Activity mContext) {
		Toolbar TB = (Toolbar) mContext.findViewById(R.id.my_awesome_toolbar);
		int position = Integer.valueOf(PreferenceManager.getDefaultSharedPreferences(mContext).getString((String) UISettingsFragment.THEME_SELECTION, "1"));
		if (pos != null) {
			position = Integer.valueOf(pos.toString());
		}
		int theme;
		switch (position) {
			case 0:
				if (TB != null){TB.setBackgroundResource(R.color.action_bar_Yellow);}
				theme = R.style.Yellow;
				mContext.setTheme(R.style.Yellow);
				break;
			default:
			case 1:
				if (TB != null){TB.setBackgroundResource(R.color.action_bar_Blue);}
				theme = R.style.Blue;
				mContext.setTheme(R.style.Blue);
				break;
			case 2:
				if (TB != null){TB.setBackgroundResource(R.color.action_bar_Pink);}
				theme = R.style.Pink;
				mContext.setTheme(R.style.Pink);
				break;
			case 3:
				if (TB != null){TB.setBackgroundResource(R.color.action_bar_purple);}
				theme = R.style.Purple;
				mContext.setTheme(R.style.Purple);
				break;
			case 4:
				if (TB != null){TB.setBackgroundResource(R.color.action_bar_Teal);}
				theme = R.style.Teal;
				mContext.setTheme(R.style.Teal);
				break;
		}
		mContext.getTheme().applyStyle(theme,true);
	}

	@Override
	protected boolean isValidFragment(String fragmentName)
	{
		return true;
	}

	public static class GeneralSettingsFragment extends PreferenceFragment
	{
		@Override
		public void onCreate(Bundle savedInstanceState)
		{
			super.onCreate(savedInstanceState);

			addPreferencesFromResource(R.xml.settings_emu_fragment);
			
			PreferenceGroup prefGroup = getPreferenceScreen();
			for(int i = 0; i < prefGroup.getPreferenceCount(); i++)
			{
				Preference pref = prefGroup.getPreference(i);
				if(pref instanceof CheckBoxPreference)
				{
					CheckBoxPreference checkBoxPref = (CheckBoxPreference)pref;
					checkBoxPref.setChecked(SettingsManager.getPreferenceBoolean(checkBoxPref.getKey()));
				}
			}
		}

		@Override
		public void onDestroy()
		{
			PreferenceGroup prefGroup = getPreferenceScreen();
			for(int i = 0; i < prefGroup.getPreferenceCount(); i++)
			{
				Preference pref = prefGroup.getPreference(i);
				if(pref instanceof CheckBoxPreference)
				{
					CheckBoxPreference checkBoxPref = (CheckBoxPreference)pref;
					SettingsManager.setPreferenceBoolean(checkBoxPref.getKey(), checkBoxPref.isChecked());
				}
			}

			super.onDestroy();
		}
	}

	public static class UISettingsFragment extends PreferenceFragment
	{
		public static CharSequence THEME_SELECTION = "ui.theme_selection";
		@Override
		public void onCreate(Bundle savedInstanceState)
		{
			super.onCreate(savedInstanceState);
            
            addPreferencesFromResource(R.xml.settings_ui_fragment);
            
            final Preference button_f = (Preference)getPreferenceManager().findPreference("ui.clearfolder");
            if (button_f != null) {
                button_f.setOnPreferenceClickListener(new Preference.OnPreferenceClickListener() {
                    @Override
                    public boolean onPreferenceClick(Preference arg0) {
                        MainActivity.resetDirectory();
                        getPreferenceScreen().removePreference(button_f);
                        return true;
                    }
                });
            }
            final Preference button_c = (Preference)getPreferenceManager().findPreference("ui.clearcache");
            if (button_c != null) {
                button_c.setOnPreferenceClickListener(new Preference.OnPreferenceClickListener() {
                    @Override
                    public boolean onPreferenceClick(Preference arg0) {
                        MainActivity.clearCache();
                        getPreferenceScreen().removePreference(button_c);
                        return true;
                    }
                });
            }

			final Preference button_t = (Preference)getPreferenceManager().findPreference(THEME_SELECTION);
			if (button_t != null) {
				button_t.setOnPreferenceChangeListener(new Preference.OnPreferenceChangeListener() {
					@Override
					public boolean onPreferenceChange(Preference preference, Object value) {
						//int index = listPreference.findIndexOfValue(stringValue);
						ChangeTheme(value, getActivity());
						return true;
					}
				});
				bindPreferenceSummaryToValue(button_t);
			}
		}

		@Override
		public void onDestroy()
		{
			super.onDestroy();
		}
		private void bindPreferenceSummaryToValue(Preference preference) {
			// Trigger the listener immediately with the preference's current value.
			preference.getOnPreferenceChangeListener().onPreferenceChange(preference,
					PreferenceManager
							.getDefaultSharedPreferences(preference.getContext())
							.getString(preference.getKey(), ""));
		}
	}
}
