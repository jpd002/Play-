package com.virtualapplications.play.settings;

import android.os.Bundle;
import android.preference.CheckBoxPreference;
import android.preference.ListPreference;
import android.preference.Preference;
import android.preference.PreferenceFragment;
import android.preference.PreferenceGroup;

import com.virtualapplications.play.R;
import com.virtualapplications.play.SettingsManager;

import static com.virtualapplications.play.Constants.PREF_EMU_AUDIO_BUFFERSIZE;
import static com.virtualapplications.play.Constants.PREF_EMU_VIDEO_PRESENTATIONMODE;
import static com.virtualapplications.play.Constants.PREF_EMU_VIDEO_RESFACTOR;

public class EmulatorSettingsFragment extends PreferenceFragment
{
	@Override
	public void onCreate(Bundle savedInstanceState)
	{
		super.onCreate(savedInstanceState);
		addPreferencesFromResource(R.xml.preferences_emu);
		writeToPreferences(getPreferenceScreen());
		ListPreference pref = (ListPreference)findPreference(PREF_EMU_VIDEO_RESFACTOR);
		pref.setOnPreferenceChangeListener(new Preference.OnPreferenceChangeListener()
		{
			@Override
			public boolean onPreferenceChange(Preference preference, Object value)
			{
				String stringValue = value.toString();
				ListPreference listBoxPref = (ListPreference)preference;
				listBoxPref.setSummary(stringValue + "x");
				return true;
			}
		});
		pref.setSummary(pref.getEntry());
		ListPreference presentationmode_pref = (ListPreference)findPreference(PREF_EMU_VIDEO_PRESENTATIONMODE);
		presentationmode_pref.setOnPreferenceChangeListener(new Preference.OnPreferenceChangeListener()
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
		presentationmode_pref.setSummary(presentationmode_pref.getEntry());

		ListPreference spublockcount_pref = (ListPreference)findPreference(PREF_EMU_AUDIO_BUFFERSIZE);
		spublockcount_pref.setOnPreferenceChangeListener(new Preference.OnPreferenceChangeListener()
		{
			@Override
			public boolean onPreferenceChange(Preference preference, Object value)
			{
				preference.setSummary(value.toString());
				return true;
			}
		});
		spublockcount_pref.setSummary(spublockcount_pref.getEntry());
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
				String val = String.valueOf(SettingsManager.getPreferenceInteger(listBoxPref.getKey()));
				listBoxPref.setValue(val);
			}
			else if(pref instanceof PreferenceGroup)
			{
				writeToPreferences((PreferenceGroup)pref);
			}
		}
	}
}

