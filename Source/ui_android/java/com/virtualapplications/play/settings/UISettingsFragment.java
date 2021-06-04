package com.virtualapplications.play.settings;

import android.content.SharedPreferences;
import android.os.Bundle;
import android.widget.Toast;

import com.virtualapplications.play.R;

import java.io.File;

import androidx.preference.ListPreference;
import androidx.preference.Preference;
import androidx.preference.PreferenceCategory;
import androidx.preference.PreferenceFragmentCompat;
import androidx.preference.PreferenceManager;

import static com.virtualapplications.play.Constants.PREF_UI_CATEGORY_STORAGE;
import static com.virtualapplications.play.Constants.PREF_UI_CLEAR_CACHE;
import static com.virtualapplications.play.Constants.PREF_UI_CLEAR_UNAVAILABLE;
import static com.virtualapplications.play.Constants.PREF_UI_RESCAN;
import static com.virtualapplications.play.Constants.PREF_UI_THEME_SELECTION;

public class UISettingsFragment extends PreferenceFragmentCompat
		implements Preference.OnPreferenceClickListener
{
	@Override
	public void onCreatePreferences(Bundle savedInstanceState, String rootKey)
	{
		addPreferencesFromResource(R.xml.preferences_ui);

		findPreference(PREF_UI_RESCAN).setOnPreferenceClickListener(this);
		findPreference(PREF_UI_CLEAR_UNAVAILABLE).setOnPreferenceClickListener(this);
		findPreference(PREF_UI_CLEAR_CACHE).setOnPreferenceClickListener(this);

		final ListPreference themeSelectionPref = findPreference(PREF_UI_THEME_SELECTION);
		themeSelectionPref.setOnPreferenceChangeListener((preference, value) -> {
			final int index = Integer.parseInt(value.toString());
			final ListPreference listBoxPref = (ListPreference)preference;

			listBoxPref.setSummary(listBoxPref.getEntries()[index]);
			return true;
		});
		themeSelectionPref.setSummary(themeSelectionPref.getEntry());
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
	public boolean onPreferenceClick(final Preference preference)
	{
		final SharedPreferences prefs = PreferenceManager.getDefaultSharedPreferences(getActivity());
		final PreferenceCategory preferenceCategory = findPreference(PREF_UI_CATEGORY_STORAGE);
		switch(preference.getKey())
		{
		case PREF_UI_RESCAN:
			prefs.edit().putBoolean(PREF_UI_RESCAN, true).apply();
			preferenceCategory.removePreference(preference);
			Toast.makeText(getActivity(), "Rescanning storage...", Toast.LENGTH_SHORT).show();
			return true;
		case PREF_UI_CLEAR_UNAVAILABLE:
			prefs.edit().putBoolean(PREF_UI_CLEAR_UNAVAILABLE, true).apply();
			preferenceCategory.removePreference(preference);
			Toast.makeText(getActivity(), "Removing unavailable games...", Toast.LENGTH_SHORT).show();
			return true;
		case PREF_UI_CLEAR_CACHE:
			clearCoverCache();
			preferenceCategory.removePreference(preference);
			Toast.makeText(getActivity(), "Clearing cover cache...", Toast.LENGTH_SHORT).show();
			return true;
		default:
			return false;
		}
	}
}
