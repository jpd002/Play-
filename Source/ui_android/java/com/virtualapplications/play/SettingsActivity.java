package com.virtualapplications.play;

import android.content.SharedPreferences;
import android.os.Bundle;
import android.view.MenuItem;

import androidx.appcompat.app.AppCompatActivity;
import androidx.appcompat.widget.Toolbar;
import androidx.core.app.ActivityCompat;
import androidx.preference.PreferenceFragmentCompat;
import androidx.preference.PreferenceManager;

import static com.virtualapplications.play.Constants.PREF_UI_THEME_SELECTION;

public class SettingsActivity extends AppCompatActivity
		implements SharedPreferences.OnSharedPreferenceChangeListener
{
	private SharedPreferences prefs;
	private Toolbar toolbar;

	@Override
	public void onCreate(final Bundle savedInstanceState)
	{
		super.onCreate(savedInstanceState);
		prefs = PreferenceManager.getDefaultSharedPreferences(this);
		prefs.registerOnSharedPreferenceChangeListener(this);

		setContentView(R.layout.activity_settings);

		toolbar = findViewById(R.id.settings_toolbar);
		setSupportActionBar(toolbar);
		toolbar.setNavigationOnClickListener(v -> onBackPressed());

		setDefaultValues(R.xml.preferences_emu);
		setDefaultValues(R.xml.preferences_ui);

		if(savedInstanceState == null)
		{
			getSupportFragmentManager().beginTransaction()
					.replace(R.id.settings_fragment_holder, new MainSettingsFragment())
					.commit();
		}
	}

	private void setDefaultValues(final int resId)
	{
		PreferenceManager.setDefaultValues(this, resId, true);
	}

	@Override
	public void onDestroy()
	{
		prefs.unregisterOnSharedPreferenceChangeListener(this);
		super.onDestroy();
		SettingsManager.save();
	}

	@Override
	public boolean onOptionsItemSelected(final MenuItem item)
	{
		final int id = item.getItemId();
		if(id == android.R.id.home)
		{
			if(getSupportFragmentManager().getBackStackEntryCount() == 0)
			{
				finish();
			}
			else
			{
				getSupportFragmentManager().popBackStack();
			}
		}

		return super.onOptionsItemSelected(item);
	}

	@Override
	protected void onResume()
	{
		super.onResume();
		ThemeManager.applyTheme(this, toolbar);
	}

	@Override
	public void onSharedPreferenceChanged(final SharedPreferences prefs, final String key)
	{
		if(key.equals(PREF_UI_THEME_SELECTION))
		{
			ThemeManager.applyTheme(this, toolbar);
			ActivityCompat.recreate(this);
		}
	}

	public static class MainSettingsFragment extends PreferenceFragmentCompat
	{
		@Override
		public void onCreatePreferences(final Bundle savedInstanceState, final String rootKey)
		{
			addPreferencesFromResource(R.xml.preferences);
		}
	}
}
