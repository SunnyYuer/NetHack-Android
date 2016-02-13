package com.nethackff;

import android.app.Activity;
import android.util.Log;
import android.os.Environment;
//import java.io.BufferedReader;
import java.io.BufferedInputStream;
import java.io.BufferedOutputStream;
import java.io.File;
import java.io.FileInputStream;
import java.io.FileOutputStream;
//import java.io.FileDescriptor;
//import java.io.FileInputStream;
//import java.io.FileNotFoundException;
import java.io.IOException;
import java.lang.reflect.InvocationTargetException;
import java.lang.reflect.Method;

public class NetHackFileHelpers
{
	static public void mkdir(String dirname)
	{
		// This is how it used to be done, but it's probably not a good idea
		// to rely on some external command in a hardcoded path... /FF
		//	doCommand("/system/bin/mkdir", dirname, "");

		boolean status = new File(dirname).mkdirs();

		// Probably good to keep the debug spew here for now. /FF
		if(status)
		{
			Log.i("NetHackDbg", "Created dir '" + dirname + "'");

			// Probably best to keep stuff accessible, for now.
			chmod(dirname, 0777);
		}
		else
		{
			Log.i("NetHackDbg", "Failed to create dir '" + dirname + "', may already exist");
		}
	}

	static public void chmod(String filename, int permissions)
	{
		// This was a bit problematic - there is an android.os.FileUtils.setPermissions()
		// function, but apparently that is not a part of the supported interface.
		// I found some other options:
		// - java.io.setReadOnly() exists, but seems limited.
		// - java.io.File.setWritable() is a part of Java 1.6, but doesn't seem to exist in Android.
		// - java.nio.file.attribute.PosixFilePermission also doesn't seem to exist under Android.
		// - doCommand("/system/bin/chmod", permissions, filename) was what I used to do, but it was crashing for some.
		// I don't think these permissions are actually critical for anything in the application,
		// so for now, we will try to use the undocumented function and just be careful to catch any exceptions
		// and print some output spew. /FF
		
		try
		{
		    Class<?> fileUtils = Class.forName("android.os.FileUtils");
		    Method setPermissions =
		        fileUtils.getMethod("setPermissions", String.class, int.class, int.class, int.class);
		    int a = (Integer) setPermissions.invoke(null, filename, permissions, -1, -1);
		    if(a != 0)
		    {
		    	// This will probably always happen now when running from SD card (or a Samsung phone),
		    	// so don't generate error spew.
				//	Log.i("NetHackDbg", "android.os.FileUtils.setPermissions() returned " + a + " for '" + filename + "', probably didn't work.");
		    }
		}
		catch(ClassNotFoundException e)
		{
			Log.i("NetHackDbg", "android.os.FileUtils.setPermissions() failed - ClassNotFoundException.");
		}
		catch(IllegalAccessException e)
		{
			Log.i("NetHackDbg", "android.os.FileUtils.setPermissions() failed - IllegalAccessException.");
		}
		catch(InvocationTargetException e)
		{
			Log.i("NetHackDbg", "android.os.FileUtils.setPermissions() failed - InvocationTargetException.");
		}
		catch(NoSuchMethodException e)
		{
			Log.i("NetHackDbg", "android.os.FileUtils.setPermissions() failed - NoSuchMethodException.");
		}
	}

	static public void copyFileRaw(String srcname, String destname) throws IOException
	{
		File newasset = new File(destname);
		File srcfile = new File(srcname);
		try
		{
			newasset.createNewFile();
			BufferedOutputStream out = new BufferedOutputStream(new FileOutputStream(newasset));
			BufferedInputStream in = new BufferedInputStream(new FileInputStream(srcfile));
			int b;
			while((b = in.read()) != -1)
			{
				out.write(b);
			}
			out.flush();
			out.close();
			in.close();
		}
		catch(IOException ex)
		{
			throw ex;
		}
	}

	static public boolean checkExternalStorageReady()
	{
		String state = Environment.getExternalStorageState();
		if(Environment.MEDIA_MOUNTED.equals(state))
		{
			return true;
		}
		else
		{
			return false;
		}			
	}

	static public String constructAppDirName(Activity activity, boolean installexternal, boolean useobsoletepath)
	{
		if(installexternal)
		{
			// This turns out to have been added in API version 8 (Android 2.2), so it won't work
			// on older devices, unfortunately:
			//	File externalFile = activityNetHackApp.getExternalFilesDir(null);
			//	return externalFile.getAbsolutePath();
			// We use the "official workaround" (see
			// http://developer.android.com/guide/topics/data/data-storage.html#filesExternalits):
			File externalFileRoot = android.os.Environment.getExternalStorageDirectory();
			if(externalFileRoot != null)
			{
				String pkgName = activity.getPackageName();	// "com.nethackff" or something

				return externalFileRoot.getAbsolutePath() + "/Android/data/" + pkgName + "/files";
			}
			// Not sure - this "else" case is not good, probably some unexpected SD card unavailability change.
			// We fall back to the internal path, which may at least give us some chance of recovering.
		}
		else if(useobsoletepath)
		{
			return "/data/data/com.nethackff";
		}
		return activity.getFilesDir().getAbsolutePath(); 
	}
}
