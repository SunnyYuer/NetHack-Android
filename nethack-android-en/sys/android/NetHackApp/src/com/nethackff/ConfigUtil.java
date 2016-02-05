package com.nethackff;

import java.io.IOException;

import android.app.AlertDialog;
import android.content.Context;
import android.content.DialogInterface;
import android.text.method.SingleLineTransformationMethod;
import android.widget.EditText;

public class ConfigUtil
{
	enum ImportExportOperation
	{
		IMPORT_CONFIG,
		EXPORT_CONFIG,
		IMPORT_PREFERENCES,
		EXPORT_PREFERENCES,
		IMPORT_KEYBINDINGS,
		EXPORT_KEYBINDINGS,
		EXPORT_EVERYTHING
	}
	public static void importExportDialog(final Context context, final ImportExportOperation operation)
	{
		final AlertDialog.Builder builder = new AlertDialog.Builder(context);
		final EditText input = new EditText(context);
		input.setTransformationMethod(SingleLineTransformationMethod.getInstance());
		switch(operation)
		{
		case IMPORT_CONFIG:
			input.getText().append(context.getString(R.string.config_defaultfile));
			builder.setTitle(R.string.configimport_title);
			builder.setMessage(R.string.configimport_msg);
			break;
		case EXPORT_CONFIG:
			input.getText().append(context.getString(R.string.config_defaultfile));
			builder.setTitle(R.string.configexport_title);
			builder.setMessage(R.string.configexport_msg);
			break;
		case IMPORT_PREFERENCES:
			input.getText().append(context.getString(R.string.preferences_defaultfile));
			builder.setTitle(R.string.preferences_import_title);
			builder.setMessage(R.string.preferences_import_message);
			break;
		case EXPORT_PREFERENCES:
			input.getText().append(context.getString(R.string.preferences_defaultfile));
			builder.setTitle(R.string.preferences_export_title);
			builder.setMessage(R.string.preferences_export_message);
			break;
		case IMPORT_KEYBINDINGS:
			input.getText().append(context.getString(R.string.keybindings_defaultfile));
			builder.setTitle(R.string.keybindings_import_title);
			builder.setMessage(R.string.keybindings_import_message);
			break;
		case EXPORT_KEYBINDINGS:
			input.getText().append(context.getString(R.string.keybindings_defaultfile));
			builder.setTitle(R.string.keybindings_export_title);
			builder.setMessage(R.string.keybindings_export_message);
			break;
		case EXPORT_EVERYTHING:
			input.getText().append(context.getString(R.string.backup_default_directory));
			builder.setTitle(R.string.backup_everything);
			builder.setMessage(R.string.keybindings_export_message);
			break;
		}

		builder.setView(input);
		builder.setPositiveButton(R.string.dialog_OK, new DialogInterface.OnClickListener()
		{
			public void onClick(DialogInterface d, int whichbutton)
			{
				String value = input.getText().toString();
				configImportExport(context, value, operation);
			}
		});
		builder.setNegativeButton(R.string.dialog_Cancel, null);

		builder.show();
	}
	
	private static void configImportExport(Context context, String filename, ImportExportOperation operation)
	{
		switch(operation)
		{
		case IMPORT_CONFIG:
			configImport(context, filename);
			break;
		case EXPORT_CONFIG:
			configExport(context, filename);
			break;
		case IMPORT_PREFERENCES:
			preferencesImport(context, filename);
			break;
		case EXPORT_PREFERENCES:
			preferencesExport(context, filename);
			break;
		case IMPORT_KEYBINDINGS:
			keybindingsImport(context, filename);
			break;
		case EXPORT_KEYBINDINGS:
			keybindingsExport(context, filename);
			break;
		case EXPORT_EVERYTHING:
			everythingExport(context, filename);
			break;
		}
	}

	private static void configImport(Context context, String inname)
	{
		try
		{
			NetHackFileHelpers.copyFileRaw(inname, getNetHackDir() + "/.nethackrc"); 

			AlertDialog.Builder alert = new AlertDialog.Builder(context);  
			alert.setTitle(R.string.dialog_Success);
			alert.setMessage(context.getString(R.string.configimport_success) + " '" + inname + "'. " + context.getString(R.string.configimport_success2));
			alert.show();
		}
		catch(IOException e)
		{
			AlertDialog.Builder alert = new AlertDialog.Builder(context);  
			alert.setTitle(R.string.dialog_Error);
			alert.setMessage(context.getString(R.string.configimport_failed) + " '" + inname + "'. " + context.getString(R.string.configimport_failed2));
			alert.show();
		}
	}
	
	private static void preferencesImport(Context context, String inname)
	{
		try
		{
			String lPreferencesPath = context.getApplicationContext().getFilesDir().getAbsolutePath() + "/../shared_prefs/com.nethackff_preferences.xml";
			NetHackFileHelpers.copyFileRaw(inname, lPreferencesPath);

			AlertDialog.Builder alert = new AlertDialog.Builder(context);  
			alert.setTitle(R.string.dialog_Success);
			alert.setMessage(context.getString(R.string.preferences_import_success) + " '" + inname + "'. ");
			alert.show();
		}
		catch(IOException e)
		{
			AlertDialog.Builder alert = new AlertDialog.Builder(context);  
			alert.setTitle(R.string.dialog_Error);
			alert.setMessage(context.getString(R.string.configimport_failed) + " '" + inname + "'. " + context.getString(R.string.configimport_failed2));
			alert.show();
		}
	}
	
	private static void keybindingsImport(Context context, String inname)
	{
		try
		{
			String lPreferencesPath = context.getApplicationContext().getFilesDir().getAbsolutePath() + "/../shared_prefs/keyMapPreferences.xml";
			NetHackFileHelpers.copyFileRaw(inname, lPreferencesPath);

			AlertDialog.Builder alert = new AlertDialog.Builder(context);  
			alert.setTitle(R.string.dialog_Success);
			alert.setMessage(context.getString(R.string.keybindings_import_success) + " '" + inname + "'. ");
			alert.show();
		}
		catch(IOException e)
		{
			AlertDialog.Builder alert = new AlertDialog.Builder(context);  
			alert.setTitle(R.string.dialog_Error);
			alert.setMessage(context.getString(R.string.configimport_failed) + " '" + inname + "'. " + context.getString(R.string.configimport_failed2));
			alert.show();
		}
	}
	
	private static void configExport(Context context, String outname)
	{
		try
		{
			NetHackFileHelpers.copyFileRaw(getNetHackDir() + "/.nethackrc", outname);

			AlertDialog.Builder alert = new AlertDialog.Builder(context);  
			alert.setTitle(R.string.dialog_Success);
			alert.setMessage(context.getString(R.string.configexport_success) + " '" + outname + "'.");
			alert.show();
		}
		catch(IOException e)
		{
			AlertDialog.Builder alert = new AlertDialog.Builder(context);  
			alert.setTitle(R.string.dialog_Error);
			alert.setMessage(context.getString(R.string.configexport_failed) + " '" + outname + "'.");
			alert.show();
		}
	}
	
	private static void preferencesExport(Context context, String outname)
	{
		try
		{
			String lPreferencesPath = context.getApplicationContext().getFilesDir().getAbsolutePath() + "/../shared_prefs/com.nethackff_preferences.xml";
			NetHackFileHelpers.copyFileRaw(lPreferencesPath,  outname);

			AlertDialog.Builder alert = new AlertDialog.Builder(context);  
			alert.setTitle(R.string.dialog_Success);
			alert.setMessage(context.getString(R.string.configexport_success) + " '" + outname + "'.");
			alert.show();
		}
		catch(IOException e)
		{
			AlertDialog.Builder alert = new AlertDialog.Builder(context);  
			alert.setTitle(R.string.dialog_Error);
			alert.setMessage(context.getString(R.string.configexport_failed) + " '" + outname + "'.");
			alert.show();
		}
	}
	
	private static void keybindingsExport(Context context, String outname)
	{
		try
		{
			String lPreferencesPath = context.getApplicationContext().getFilesDir().getAbsolutePath() + "/../shared_prefs/keyMapPreferences.xml";
			NetHackFileHelpers.copyFileRaw(lPreferencesPath,  outname);

			AlertDialog.Builder alert = new AlertDialog.Builder(context);  
			alert.setTitle(R.string.dialog_Success);
			alert.setMessage(context.getString(R.string.keybindings_export_success) + " '" + outname + "'.");
			alert.show();
		}
		catch(IOException e)
		{
			AlertDialog.Builder alert = new AlertDialog.Builder(context);  
			alert.setTitle(R.string.dialog_Error);
			alert.setMessage(context.getString(R.string.configexport_failed) + " '" + outname + "'.");
			alert.show();
		}
	}
	
	private static void everythingExport(Context context, String outdir)
	{
		if (!outdir.endsWith("/"))
		{
			outdir += "/";
		}
		
		try
		{
			String lKeyMapPreferencesPath = context.getApplicationContext().getFilesDir().getAbsolutePath() + "/../shared_prefs/keyMapPreferences.xml";
			NetHackFileHelpers.copyFileRaw(lKeyMapPreferencesPath,  outdir + "keyMapPreferences.xml");
			
			String lPreferencesPath = context.getApplicationContext().getFilesDir().getAbsolutePath() + "/../shared_prefs/com.nethackff_preferences.xml";
			NetHackFileHelpers.copyFileRaw(lPreferencesPath,  outdir + "com.nethackff_preferences.xml");

			NetHackFileHelpers.copyFileRaw(getNetHackDir() + "/.nethackrc", outdir + "NetHack.cnf");
			AlertDialog.Builder alert = new AlertDialog.Builder(context);  
			alert.setTitle(R.string.dialog_Success);
			alert.setMessage(context.getString(R.string.everything_export_success) + " '" + outdir + "'.");
			alert.show();
		}
		catch(IOException e)
		{
			AlertDialog.Builder alert = new AlertDialog.Builder(context);  
			alert.setTitle(R.string.dialog_Error);
			alert.setMessage(context.getString(R.string.configexport_failed) + " '" + outdir + "'.");
			alert.show();
		}
	}
	
	public static String getNetHackDir()
	{
		return NetHackGame.appDir + "/nethackdir"; 
	}
}
