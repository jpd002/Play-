package com.virtualapplications.play;

import android.os.*;
import android.preference.*;
import android.widget.*;
import java.util.*;

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

			addPreferencesFromResource(R.xml.settings_general_fragment);
			
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
}
