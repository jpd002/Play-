package com.virtualapplications.play.settings;

import android.os.Bundle;

import com.virtualapplications.play.R;
import com.virtualapplications.play.SettingsManager;

import androidx.preference.ListPreference;
import androidx.preference.Preference;
import androidx.preference.PreferenceFragmentCompat;
import androidx.preference.PreferenceGroup;
import androidx.preference.SwitchPreferenceCompat;

public class EmulatorSettingsFragment extends PreferenceFragmentCompat
{
	@Override
	public void onCreatePreferences(Bundle savedInstanceState, String rootKey)
	{
		addPreferencesFromResource(R.xml.preferences_emu);
		writeToPreferences(getPreferenceScreen());
	}

	@Override
	public void onDestroy()
	{
		readFromPreferences(getPreferenceScreen());
		super.onDestroy();
	}

	private void readFromPreferences(final PreferenceGroup prefGroup)
	{
		for(int i = 0; i < prefGroup.getPreferenceCount(); i++)
		{
			final Preference pref = prefGroup.getPreference(i);
			if(pref instanceof SwitchPreferenceCompat)
			{
				final SwitchPreferenceCompat switchPref = (SwitchPreferenceCompat)pref;
				SettingsManager.setPreferenceBoolean(switchPref.getKey(), switchPref.isChecked());
			}
			else if(pref instanceof ListPreference)
			{
				final ListPreference listBoxPref = (ListPreference)pref;
				final int val = Integer.parseInt(listBoxPref.getValue());
				SettingsManager.setPreferenceInteger(listBoxPref.getKey(), val);
			}
			else if(pref instanceof PreferenceGroup)
			{
				readFromPreferences((PreferenceGroup)pref);
			}
		}
	}

	private void writeToPreferences(final PreferenceGroup prefGroup)
	{
		for(int i = 0; i < prefGroup.getPreferenceCount(); i++)
		{
			final Preference pref = prefGroup.getPreference(i);
			if(pref instanceof SwitchPreferenceCompat)
			{
				final SwitchPreferenceCompat switchPref = (SwitchPreferenceCompat)pref;
				switchPref.setChecked(SettingsManager.getPreferenceBoolean(switchPref.getKey()));
			}
			else if(pref instanceof ListPreference)
			{
				final ListPreference listBoxPref = (ListPreference)pref;
				final String val = String.valueOf(SettingsManager.getPreferenceInteger(listBoxPref.getKey()));
				listBoxPref.setValue(val);
			}
			else if(pref instanceof PreferenceGroup)
			{
				writeToPreferences((PreferenceGroup)pref);
			}
		}
	}
}
