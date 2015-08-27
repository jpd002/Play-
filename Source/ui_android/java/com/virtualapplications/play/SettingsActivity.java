package com.virtualapplications.play;

import android.app.Activity;
import android.content.SharedPreferences;
import android.os.*;
import android.preference.*;
import android.view.LayoutInflater;
import android.view.View;
import android.widget.*;
import java.util.*;
import android.support.v7.widget.Toolbar;
import android.graphics.Point;

public class SettingsActivity extends PreferenceActivity implements SharedPreferences.OnSharedPreferenceChangeListener
{
	@Override
	public void onCreate(Bundle savedInstanceState)
	{
		super.onCreate(savedInstanceState);
		SharedPreferences prefs = PreferenceManager.getDefaultSharedPreferences(this);
		prefs.registerOnSharedPreferenceChangeListener(this);
	}
	
	@Override
	public void onDestroy()
	{
		SharedPreferences prefs = PreferenceManager.getDefaultSharedPreferences(this);
		prefs.unregisterOnSharedPreferenceChangeListener(this);
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
		LinearLayout root = (LinearLayout)findViewById(android.R.id.list).getParent().getParent().getParent();
		Toolbar bar = (Toolbar)LayoutInflater.from(this).inflate(R.layout.settings_toolbar, null, false);
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
		ThemeManager.applyTheme(this);
	}

	@Override
	protected boolean isValidFragment(String fragmentName)
	{
		return true;
	}

	@Override
	public void onSharedPreferenceChanged(SharedPreferences prefs, String key)
	{
		if(key.equals(ThemeManager.THEME_SELECTION))
		{
			ThemeManager.applyTheme(this);
		}
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
		}

		@Override
		public void onDestroy()
		{
			super.onDestroy();
		}
	}
}
