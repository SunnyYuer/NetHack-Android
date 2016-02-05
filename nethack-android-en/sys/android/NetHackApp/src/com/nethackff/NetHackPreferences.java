package com.nethackff;

import java.io.File;

import com.nethackff.configeditor.ConfigEditor;

import android.content.Intent;
import android.net.Uri;
import android.os.Bundle;
import android.preference.Preference;
import android.preference.PreferenceActivity;

public class NetHackPreferences extends PreferenceActivity
{
	public void onCreate(Bundle savedInstanceState)
	{
		super.onCreate(savedInstanceState);

		// Hmm, caused an exception for some reason.
		//	requestWindowFeature(Window.FEATURE_NO_TITLE);

		addPreferencesFromResource(R.xml.preferences);

		NetHackListPreferenceTileSet tileSetList = (NetHackListPreferenceTileSet)findPreference("TileSet"); 

		setTileSetPreference(tileSetList);
		
		setKeyBindPreferenceIntent();
		
		setConfigOptionsPreferenceIntent();
		
		setBackupImportSettingsIntent();
	}

	private void setBackupImportSettingsIntent()
	{
		Preference backupImportSettings = findPreference("backupImportSettings");
		Intent backupImportSettingsIntent = new Intent(this, BackupImportSettings.class);
		backupImportSettings.setIntent(backupImportSettingsIntent);
	}

	private void setConfigOptionsPreferenceIntent()
	{
		Preference configOptions = findPreference("editConfig");
		File file = new File(ConfigUtil.getNetHackDir() + "/.nethackrc");
		Uri uri = Uri.fromFile(file);
		Intent editConfigIntent = new Intent(Intent.ACTION_VIEW ,uri);
		editConfigIntent.setDataAndType(uri, "text/plain"); 
		editConfigIntent.setClass(this, ConfigEditor.class);
		configOptions.setIntent(editConfigIntent);
	}

	private void setKeyBindPreferenceIntent()
	{
		Preference keyBindPreference = findPreference("keyBindings");
		Intent intent = new Intent(getApplicationContext(), KeyBindingListActivity.class);
		keyBindPreference.setIntent(intent);
	}

	private void setTileSetPreference(NetHackListPreferenceTileSet tileSetList) {
		String tilesetnames[] = this.getIntent().getExtras().getStringArray("TileSetNames");
		String tilesetvalues[] = this.getIntent().getExtras().getStringArray("TileSetValues");
		String tilesetinfo[] = this.getIntent().getExtras().getStringArray("TileSetInfo");
		
		tileSetList.setEntryValues(tilesetvalues);
		tileSetList.setEntries(tilesetnames);
		tileSetList.setTileSetInfo(tilesetinfo);
		tileSetList.setDefaultValue(tilesetvalues[0]);
		
		tileSetList.setInfoFromValue();
	}
}
