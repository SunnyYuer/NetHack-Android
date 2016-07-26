package com.yuer.NetHack;

import com.yuer.NetHack.R;

import android.content.Intent;
import android.content.SharedPreferences;
import android.content.SharedPreferences.OnSharedPreferenceChangeListener;
import android.os.Bundle;
import android.preference.Preference;
import android.preference.PreferenceActivity;
import android.preference.PreferenceCategory;
import android.preference.PreferenceScreen;
import android.view.Window;
import com.yuer.NetHack.TilesetPreference;

public class Settings extends PreferenceActivity implements OnSharedPreferenceChangeListener
{
	private TilesetPreference mTilesetPref;

	// ____________________________________________________________________________________
	@Override
	protected void onCreate(Bundle savedInstanceState)
	{
		// turn off the window's title bar
		requestWindowFeature(Window.FEATURE_NO_TITLE);

		super.onCreate(savedInstanceState);

		addPreferencesFromResource(R.xml.preferences);
	}

	// ____________________________________________________________________________________
	@Override
	protected void onResume()
	{
		super.onResume();

		SharedPreferences sharedPreferences = getPreferenceScreen().getSharedPreferences();

		for(int i = 0; i < 10; i++)
		{
			char idx = (char)('0' + i);
			Preference screen = findPreference("panel" + idx);

			if(screen == null)
				break;

			String name = sharedPreferences.getString("pName" + idx, "");

			screen.setTitle(name);
		}

		sharedPreferences.registerOnSharedPreferenceChangeListener(this);

		mTilesetPref = (TilesetPreference)findPreference("tilesetPreference");
		mTilesetPref.setActivity(this);
		
		if(!getApplicationContext().getResources().getBoolean(R.bool.hearseAvailable))
		{
			PreferenceCategory hearseParent = (PreferenceCategory)findPreference("advanced");
			Preference hearsePref = findPreference("hearse");
			if(hearsePref != null)
				hearseParent.removePreference(hearsePref);
		}
	}

	// ____________________________________________________________________________________
	@Override
	protected void onActivityResult(int requestCode, int resultCode, Intent data)
	{
		super.onActivityResult(requestCode, resultCode, data);
		mTilesetPref.onActivityResult(requestCode, resultCode, data);
	}

	// ____________________________________________________________________________________
	@Override
	protected void onPause()
	{
		super.onPause();
		getPreferenceScreen().getSharedPreferences().unregisterOnSharedPreferenceChangeListener(this);
	}

	// ____________________________________________________________________________________
	@Override
	public void onSharedPreferenceChanged(SharedPreferences sharedPreferences, String key)
	{
		if(key.startsWith("pName"))
		{
			char idx = key.charAt(key.length() - 1);
			Preference screen = findPreference("panel" + idx);
			String name = sharedPreferences.getString("pName" + idx, "");
			screen.setTitle(name);
		}
	}
}
