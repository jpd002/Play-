package com.virtualapplications.play;

import android.app.Activity;
import android.content.SharedPreferences;
import android.os.*;
import android.preference.*;
import android.view.LayoutInflater;
import android.view.View;
import android.widget.*;

import java.io.File;
import java.util.*;
import android.support.v7.widget.Toolbar;
import android.graphics.Point;

import com.virtualapplications.play.database.IndexingDB;

public class SettingsActivity extends PreferenceActivity implements SharedPreferences.OnSharedPreferenceChangeListener
{
	public static final String RESCAN = "ui.rescan";
	public static final String CLEAR_UNAVAILABLE = "ui.clear_unavailable";
	private static final String UI_STORAGE = "ui.storage";
	private static final String CLEAR_CACHE = "ui.clearcache";

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
	
	public static class EmulatorSettingsFragment extends PreferenceFragment
	{
		@Override
		public void onCreate(Bundle savedInstanceState)
		{
			super.onCreate(savedInstanceState);
			addPreferencesFromResource(R.xml.settings_emu_fragment);
			writeToPreferences(getPreferenceScreen());
		}

		@Override
		public void onDestroy()
		{
			readFromPreferences(getPreferenceScreen());
			super.onDestroy();
		}
		
		private void readFromPreferences(PreferenceGroup prefGroup)
		{
			for(int i = 0; i < prefGroup.getPreferenceCount(); i++)
			{
				Preference pref = prefGroup.getPreference(i);
				if(pref instanceof CheckBoxPreference)
				{
					CheckBoxPreference checkBoxPref = (CheckBoxPreference)pref;
					SettingsManager.setPreferenceBoolean(checkBoxPref.getKey(), checkBoxPref.isChecked());
				}
				else if(pref instanceof PreferenceGroup)
				{
					readFromPreferences((PreferenceGroup)pref);
				}
			}
		}
		
		private void writeToPreferences(PreferenceGroup prefGroup)
		{
			for(int i = 0; i < prefGroup.getPreferenceCount(); i++)
			{
				Preference pref = prefGroup.getPreference(i);
				if(pref instanceof CheckBoxPreference)
				{
					CheckBoxPreference checkBoxPref = (CheckBoxPreference)pref;
					checkBoxPref.setChecked(SettingsManager.getPreferenceBoolean(checkBoxPref.getKey()));
				}
				else if(pref instanceof PreferenceGroup)
				{
					writeToPreferences((PreferenceGroup)pref);
				}
			}
		}
	}

	public static class UISettingsFragment extends PreferenceFragment
	{
		@Override
		public void onCreate(Bundle savedInstanceState)
		{
			super.onCreate(savedInstanceState);
            
            addPreferencesFromResource(R.xml.settings_ui_fragment);

			final PreferenceCategory preferenceCategory = (PreferenceCategory) findPreference(UI_STORAGE);
			final Preference button_f = (Preference)getPreferenceManager().findPreference(RESCAN);
			if (button_f != null) {
				button_f.setOnPreferenceClickListener(new Preference.OnPreferenceClickListener() {
					@Override
					public boolean onPreferenceClick(Preference arg0) {
						button_f.getEditor().putBoolean(RESCAN, true).apply();
						preferenceCategory.removePreference(button_f);
						return true;
					}
				});
			}
			final Preference button_u = (Preference)getPreferenceManager().findPreference(CLEAR_UNAVAILABLE);
			if (button_u != null) {
				button_u.setOnPreferenceClickListener(new Preference.OnPreferenceClickListener() {
					@Override
					public boolean onPreferenceClick(Preference arg0) {
						button_u.getEditor().putBoolean(CLEAR_UNAVAILABLE, true).apply();
						preferenceCategory.removePreference(button_u);
						return true;
					}
				});
			}
            final Preference button_c = (Preference)getPreferenceManager().findPreference(CLEAR_CACHE);
            if (button_c != null) {
                button_c.setOnPreferenceClickListener(new Preference.OnPreferenceClickListener() {
                    @Override
                    public boolean onPreferenceClick(Preference arg0) {
						clearCoverCache();
						preferenceCategory.removePreference(button_c);
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

		private void clearCoverCache()
		{
			File dir = new File(getActivity().getExternalFilesDir(null), "covers");
			for (File file : dir.listFiles())
			{
				if (!file.isDirectory())
				{
					file.delete();
				}
			}
		}
	}
}
