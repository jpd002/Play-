package com.virtualapplications.play.settings;

import android.os.Bundle;
import android.preference.ListPreference;
import android.preference.Preference;
import android.preference.PreferenceCategory;
import android.preference.PreferenceFragment;
import android.widget.Toast;

import com.virtualapplications.play.R;

import java.io.File;

import static com.virtualapplications.play.Constants.PREF_UI_CLEAR_CACHE;
import static com.virtualapplications.play.Constants.PREF_UI_CLEAR_UNAVAILABLE;
import static com.virtualapplications.play.Constants.PREF_UI_RESCAN;
import static com.virtualapplications.play.Constants.PREF_UI_THEME_SELECTION;
import static com.virtualapplications.play.Constants.PREF_UI_CATEGORY_STORAGE;

public class UISettingsFragment extends PreferenceFragment implements Preference.OnPreferenceClickListener
{
	@Override
	public void onCreate(Bundle savedInstanceState)
	{
		super.onCreate(savedInstanceState);

		addPreferencesFromResource(R.xml.preferences_ui);

		getPreferenceManager().findPreference(PREF_UI_RESCAN).setOnPreferenceClickListener(this);
		getPreferenceManager().findPreference(PREF_UI_CLEAR_UNAVAILABLE).setOnPreferenceClickListener(this);
		getPreferenceManager().findPreference(PREF_UI_CLEAR_CACHE).setOnPreferenceClickListener(this);
		ListPreference pref = (ListPreference)findPreference(PREF_UI_THEME_SELECTION);
		pref.setOnPreferenceChangeListener(new Preference.OnPreferenceChangeListener()
		{
			@Override
			public boolean onPreferenceChange(Preference preference, Object value)
			{
				int index = Integer.parseInt(value.toString());
				ListPreference listBoxPref = (ListPreference)preference;

				listBoxPref.setSummary(listBoxPref.getEntries()[index]);
				return true;
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

		File[] files = dir.listFiles();
		if(files == null) return;

		for(File file : files)
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
		final PreferenceCategory preferenceCategory = (PreferenceCategory)findPreference(PREF_UI_CATEGORY_STORAGE);
		switch(preference.getKey())
		{
		case PREF_UI_RESCAN:
			preference.getEditor().putBoolean(PREF_UI_RESCAN, true).apply();
			preferenceCategory.removePreference(preference);
			Toast.makeText(getActivity(), "Rescanning storage.", Toast.LENGTH_SHORT).show();
			return true;
		case PREF_UI_CLEAR_UNAVAILABLE:
			preference.getEditor().putBoolean(PREF_UI_CLEAR_UNAVAILABLE, true).apply();
			preferenceCategory.removePreference(preference);
			Toast.makeText(getActivity(), "Removing unavailable games.", Toast.LENGTH_SHORT).show();
			return true;
		case PREF_UI_CLEAR_CACHE:
			clearCoverCache();
			preferenceCategory.removePreference(preference);
			Toast.makeText(getActivity(), "Clearing cover cache.", Toast.LENGTH_SHORT).show();
			return true;
		default:
			return false;
		}
	}
}
