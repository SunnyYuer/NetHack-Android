package com.nethackff;

import android.app.ListActivity;
import android.os.Bundle;
import android.view.View;
import android.widget.ArrayAdapter;
import android.widget.ListAdapter;
import android.widget.ListView;

public class BackupImportSettings extends ListActivity
{

	@Override
	public void onCreate(Bundle savedinstanceState)
	{
		super.onCreate(savedinstanceState);
		String[] configOptionEntries = getResources().getStringArray(R.array.BackupSettingsEntries);
		ListAdapter adapter = new ArrayAdapter<String>(this, android.R.layout.simple_list_item_1, configOptionEntries);
		setListAdapter(adapter);
	}
	
	@Override
	protected void onListItemClick(ListView l, View v, int position, long id)
	{
		String clickedItem = (String) l.getItemAtPosition(position);
		if (clickedItem.equals(getString(R.string.backup_everything)))
		{
			ConfigUtil.importExportDialog(this, ConfigUtil.ImportExportOperation.EXPORT_EVERYTHING);
			return;
		}
		if (clickedItem.equals(getString(R.string.backup_config)))
		{
			ConfigUtil.importExportDialog(this, ConfigUtil.ImportExportOperation.EXPORT_CONFIG);
			return;
		}
		if (clickedItem.equals(getString(R.string.backup_preferences)))
		{
			ConfigUtil.importExportDialog(this, ConfigUtil.ImportExportOperation.EXPORT_PREFERENCES);
			return;
		}
		if (clickedItem.equals(getString(R.string.backup_keybindings)))
		{
			ConfigUtil.importExportDialog(this, ConfigUtil.ImportExportOperation.EXPORT_KEYBINDINGS);
			return;
		}
		if (clickedItem.equals(getString(R.string.import_config)))
		{
			ConfigUtil.importExportDialog(this, ConfigUtil.ImportExportOperation.IMPORT_CONFIG);
			return;
		}
		if (clickedItem.equals(getString(R.string.import_preferences)))
		{
			ConfigUtil.importExportDialog(this, ConfigUtil.ImportExportOperation.IMPORT_PREFERENCES);
			return;
		}
		if (clickedItem.equals(getString(R.string.import_keybindings)))
		{
			ConfigUtil.importExportDialog(this, ConfigUtil.ImportExportOperation.IMPORT_KEYBINDINGS);
			return;
		}
	}
}
