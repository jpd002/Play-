package com.virtualapplications.play.settings;

import android.os.Bundle;

import com.virtualapplications.play.R;
import com.virtualapplications.play.SettingsManager;

import androidx.preference.CheckBoxPreference;
import androidx.preference.ListPreference;
import androidx.preference.Preference;
import androidx.preference.PreferenceFragmentCompat;
import androidx.preference.PreferenceGroup;

import static com.virtualapplications.play.Constants.PREF_EMU_AUDIO_BUFFERSIZE;
import static com.virtualapplications.play.Constants.PREF_EMU_VIDEO_PRESENTATIONMODE;
import static com.virtualapplications.play.Constants.PREF_EMU_VIDEO_RESFACTOR;

public class EmulatorSettingsFragment extends PreferenceFragmentCompat
{
	@Override
	public void onCreatePreferences(Bundle savedInstanceState, String rootKey)
	{
		addPreferencesFromResource(R.xml.preferences_emu);
		writeToPreferences(getPreferenceScreen());

		final ListPreference resFactorPref = findPreference(PREF_EMU_VIDEO_RESFACTOR);
		resFactorPref.setOnPreferenceChangeListener((preference, value) -> {
			final String stringValue = value.toString();
			final ListPreference listBoxPref = (ListPreference)preference;
			listBoxPref.setSummary(stringValue + "x");
			return true;
		});
		resFactorPref.setSummary(resFactorPref.getEntry());

		final ListPreference presentationModePref = findPreference(PREF_EMU_VIDEO_PRESENTATIONMODE);
		presentationModePref.setOnPreferenceChangeListener((preference, value) -> {
			final int index = Integer.parseInt(value.toString());
			final ListPreference listBoxPref = (ListPreference)preference;
			listBoxPref.setSummary(listBoxPref.getEntries()[index]);
			return true;
		});
		presentationModePref.setSummary(presentationModePref.getEntry());

		final ListPreference bufferSizePref = findPreference(PREF_EMU_AUDIO_BUFFERSIZE);
		bufferSizePref.setOnPreferenceChangeListener((preference, value) -> {
			preference.setSummary(value.toString());
			return true;
		});
		bufferSizePref.setSummary(bufferSizePref.getEntry());
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
			if(pref instanceof CheckBoxPreference)
			{
				final CheckBoxPreference checkBoxPref = (CheckBoxPreference)pref;
				SettingsManager.setPreferenceBoolean(checkBoxPref.getKey(), checkBoxPref.isChecked());
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
			if(pref instanceof CheckBoxPreference)
			{
				final CheckBoxPreference checkBoxPref = (CheckBoxPreference)pref;
				checkBoxPref.setChecked(SettingsManager.getPreferenceBoolean(checkBoxPref.getKey()));
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
