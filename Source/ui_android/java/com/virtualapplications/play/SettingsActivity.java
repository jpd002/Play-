package com.virtualapplications.play;

import android.content.SharedPreferences;
import android.os.*;
import android.preference.*;
import android.view.LayoutInflater;
import android.view.View;
import android.widget.*;

import java.io.File;
import java.util.*;

import android.support.v7.widget.Toolbar;

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
	protected void onPostCreate(Bundle savedInstanceState)
	{
		super.onPostCreate(savedInstanceState);
		LinearLayout root = (LinearLayout)findViewById(android.R.id.list).getParent().getParent().getParent();
		Toolbar bar = (Toolbar)LayoutInflater.from(this).inflate(R.layout.settings_toolbar, null, false);
		bar.setNavigationOnClickListener(new View.OnClickListener()
		{
			@Override
			public void onClick(View v)
			{
				onBackPressed();
			}
		});
		root.addView(bar, 0);
	}

	@Override
	protected void onResume()
	{
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
			ListPreference pref = (ListPreference)findPreference("renderer.opengl.resfactor");
			pref.setOnPreferenceChangeListener(new Preference.OnPreferenceChangeListener()
			{
				@Override
				public boolean onPreferenceChange(Preference preference, Object value)
				{
					String stringValue = value.toString();
					ListPreference listBoxPref = (ListPreference)preference;
					listBoxPref.setSummary(stringValue + "x");
					return false;
				}
			});
			pref.setSummary(pref.getEntry());
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
				else if(pref instanceof ListPreference)
				{
					ListPreference listBoxPref = (ListPreference)pref;
					int val = Integer.parseInt(listBoxPref.getValue());
					SettingsManager.setPreferenceInteger(listBoxPref.getKey(), val);
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
				else if(pref instanceof ListPreference)
				{
					ListPreference listBoxPref = (ListPreference)pref;
					int val = SettingsManager.getPreferenceInteger(listBoxPref.getKey());
					listBoxPref.setValueIndex(val - 1);
				}
				else if(pref instanceof PreferenceGroup)
				{
					writeToPreferences((PreferenceGroup)pref);
				}
			}
		}
	}

	public static class UISettingsFragment extends PreferenceFragment implements Preference.OnPreferenceClickListener
	{
		@Override
		public void onCreate(Bundle savedInstanceState)
		{
			super.onCreate(savedInstanceState);

			addPreferencesFromResource(R.xml.settings_ui_fragment);

			getPreferenceManager().findPreference(RESCAN).setOnPreferenceClickListener(this);
			getPreferenceManager().findPreference(CLEAR_UNAVAILABLE).setOnPreferenceClickListener(this);
			getPreferenceManager().findPreference(CLEAR_CACHE).setOnPreferenceClickListener(this);
			ListPreference pref = (ListPreference)findPreference("ui.theme_selection");
			pref.setOnPreferenceChangeListener(new Preference.OnPreferenceChangeListener()
			{
				@Override
				public boolean onPreferenceChange(Preference preference, Object value)
				{
					String stringValue = value.toString();
					ListPreference listBoxPref = (ListPreference)preference;
					listBoxPref.setSummary(stringValue);
					return false;
				}
			});
			pref.setSummary(pref.getEntry());
		}

		@Override
		public void onDestroy()
		{
			super.onDestroy();
		}

		private void clearCoverCache()
		{
			File dir = new File(getActivity().getExternalFilesDir(null), "covers");
			for(File file : dir.listFiles())
			{
				if(!file.isDirectory())
				{
					file.delete();
				}
			}
		}

		@Override
		public boolean onPreferenceClick(Preference preference)
		{
			final PreferenceCategory preferenceCategory = (PreferenceCategory)findPreference(UI_STORAGE);
			switch(preference.getKey())
			{
			case RESCAN:
				preference.getEditor().putBoolean(RESCAN, true).apply();
				preferenceCategory.removePreference(preference);
				Toast.makeText(getActivity(), "Rescanning storage.", Toast.LENGTH_SHORT).show();
				return true;
			case CLEAR_UNAVAILABLE:
				preference.getEditor().putBoolean(CLEAR_UNAVAILABLE, true).apply();
				preferenceCategory.removePreference(preference);
				Toast.makeText(getActivity(), "Removing unavailable games.", Toast.LENGTH_SHORT).show();
				return true;
			case CLEAR_CACHE:
				clearCoverCache();
				preferenceCategory.removePreference(preference);
				Toast.makeText(getActivity(), "Clearing cover cache.", Toast.LENGTH_SHORT).show();
				return true;
			default:
				return false;
			}
		}
	}
}
