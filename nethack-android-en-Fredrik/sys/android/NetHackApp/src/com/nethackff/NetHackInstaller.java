package com.nethackff;

import android.content.res.AssetManager;
import android.content.SharedPreferences;
import android.os.Message;
import android.preference.PreferenceManager;
import android.util.Log;
import java.io.BufferedInputStream;
import java.io.BufferedOutputStream;
import java.io.File;
import java.io.FileInputStream;
import java.io.FileOutputStream;
import java.io.IOException;

public class NetHackInstaller
{
	// TODO: Check if this ended up being necessary.
	NetHackApp activityNetHackApp;

	String appDir;

	InstallThread installThread;

	int installProgress = 0;

	protected AssetManager assetManager;

	boolean useObsoletePath = false;

	public boolean isExistingInstallationUpToDate()
	{
		return compareAsset("version.txt");
	}


	public boolean doesExistingInstallationExist()
	{
		SharedPreferences prefs = PreferenceManager.getDefaultSharedPreferences(activityNetHackApp.getBaseContext());
		if(prefs.contains("InstalledOnExternalMemory"))
		{
			return true;
		}
		return false;
	}

	public void setAppDir(boolean installexternal)
	{
		activityNetHackApp.appDir = appDir = NetHackFileHelpers.constructAppDirName(activityNetHackApp, installexternal, useObsoletePath);
	}
		
	public String getAppDir()
	{
		return appDir;
	}

	public String getNetHackDir()
	{
		return getAppDir() + "/nethackdir"; 
	}

	public NetHackInstaller(AssetManager assetman, NetHackApp activitynethackapp, boolean launchthread)
	{
		assetManager = assetman;
		activityNetHackApp = activitynethackapp;

		if(launchthread)
		{
			installThread = new InstallThread();
			installThread.start();
		}
	}

	public boolean compareAsset(String assetname)
	{
		boolean match = false;

		String destname = getAppDir() + "/" + assetname;
		File newasset = new File(destname);
		try
		{
			BufferedInputStream out = new BufferedInputStream(new FileInputStream(newasset));
			BufferedInputStream in = new BufferedInputStream(assetManager.open(assetname));
			match = true;
			while(true)
			{
				int b = in.read();
				int c = out.read();
				if(b != c)
				{
					match = false;
					break;
				}
				if(b == -1)
				{
					break;
				}
			}
			out.close();
			in.close();
		}
		catch (IOException ex)
		{
			match = false;
		}
		return match;
	}

	public void copyAsset(String assetname)
	{
		copyAsset(assetname, getAppDir() + "/" + assetname);
	}

	public void copyAsset(String srcname, String destname)
	{
		File newasset = new File(destname);
		try
		{
			newasset.createNewFile();
			BufferedOutputStream out = new BufferedOutputStream(new FileOutputStream(newasset));
			BufferedInputStream in = new BufferedInputStream(assetManager.open(srcname));
			int b;
			while((b = in.read()) != -1)
			{
				out.write(b);
			}
			out.flush();
			out.close();
			in.close();
		}
		catch (IOException ex)
		{
			//mainView.terminal.write("Failed to copy file '" + srcname + "'.\n");
		}
	}

	public void copyNetHackData()
	{
		String assets[] = null;
		try
		{
			assets = assetManager.list("nethackdir");

			for(int i = 0; i < assets.length; i++)
			{
				String destname = getNetHackDir() + "/" + assets[i]; 
				copyAsset("nethackdir/" + assets[i], destname);
				NetHackFileHelpers.chmod(destname, 0666);

				reportProgress();
			}
		}
		catch (IOException e)
		{
			throw new RuntimeException(e.getMessage());
		}
	}

	public void reportProgress()
	{
		installProgress++;
		activityNetHackApp.handler.sendMessage(Message.obtain(activityNetHackApp.handler, NetHackApp.MSG_INSTALL_PROGRESS, installProgress, 0, null));
	}

	public int beginInstall()
	{
		int totalprogress = 0;
		String assets[] = null;
		try
		{
			assets = assetManager.list("nethackdir");
			totalprogress += assets.length;
		}
		catch (IOException e)
		{
			throw new RuntimeException(e.getMessage());
		}

		totalprogress += 8;	// MAGIC!

		return totalprogress;

	}

	public void performInstall()
	{
		installProgress = 0;

		String nethackdir = getNetHackDir();

		NetHackFileHelpers.mkdir(nethackdir);
		reportProgress();
		NetHackFileHelpers.mkdir(nethackdir + "/save");
		reportProgress();

		copyNetHackData();
		reportProgress();

		copyAsset("version.txt");
		reportProgress();
		copyAsset("NetHack.cnf", nethackdir + "/.nethackrc");
		reportProgress();
		copyAsset("charset_amiga.cnf", nethackdir + "/charset_amiga.cnf");
		reportProgress();
		copyAsset("charset_ibm.cnf", nethackdir + "/charset_ibm.cnf");
		reportProgress();
		copyAsset("charset_128.cnf", nethackdir + "/charset_128.cnf");
		reportProgress();

		activityNetHackApp.handler.sendEmptyMessage(NetHackApp.MSG_INSTALL_END);
	}

	public void install(boolean installexternal)
	{
		setAppDir(installexternal);

		int finalProgress = beginInstall();
		activityNetHackApp.handler.sendMessage(Message.obtain(activityNetHackApp.handler, NetHackApp.MSG_INSTALL_BEGIN, finalProgress, 0, null));
		performInstall();

		storePrefsInstalledOnExternalMemory(installexternal);

		if(installexternal)
		{
			// If we just installed externally, we can't be using the obsolete path, so make sure
			// we clear this out from the prefs database:
			storePrefsInstalledInObsoletePath(false);
			useObsoletePath = false;
		}
	}

	public void storePrefsInstalledOnExternalMemory(boolean installexternal)
	{
		SharedPreferences prefs = PreferenceManager.getDefaultSharedPreferences(activityNetHackApp.getBaseContext());
		SharedPreferences.Editor prefsEditor = prefs.edit();
		prefsEditor.putBoolean("InstalledOnExternalMemory", installexternal);
		prefsEditor.commit();
	}

	public void storePrefsInstalledInObsoletePath(boolean obsoletepath)
	{
		SharedPreferences prefs = PreferenceManager.getDefaultSharedPreferences(activityNetHackApp.getBaseContext());
		SharedPreferences.Editor prefsEditor = prefs.edit();
		prefsEditor.putBoolean("InstalledInObsoletePath", obsoletepath);
		prefsEditor.commit();
	}

	public boolean readPrefsInstalledInObsoletePath()
	{
		SharedPreferences prefs = PreferenceManager.getDefaultSharedPreferences(activityNetHackApp.getBaseContext());
		return prefs.getBoolean("InstalledInObsoletePath", false);
	}

	public class InstallThread extends Thread implements Runnable
	{
		public synchronized void setDialogResponse(NetHackApp.DialogResponse r)
		{
			dialogResponse = r;
		}

		public synchronized NetHackApp.DialogResponse getDialogResponse()
		{
			return dialogResponse;
		}

		protected NetHackApp.DialogResponse dialogResponse = NetHackApp.DialogResponse.Invalid;

		public boolean isExistingInstallationAvailable()
		{
			SharedPreferences prefs = PreferenceManager.getDefaultSharedPreferences(activityNetHackApp.getBaseContext());
			if(prefs.getBoolean("InstalledOnExternalMemory", false))
			{
				if(!NetHackFileHelpers.checkExternalStorageReady())
				{
					return false;
				}
			}
			return true;
		}

		public void waitForResponse()
		{
			while(getDialogResponse() == NetHackApp.DialogResponse.Invalid)
			{
				try
				{
					//handler.sendEmptyMessage(0);
					Thread.sleep(100);
				}
				catch(InterruptedException e)
				{
					throw new RuntimeException(e.getMessage());
				}
			}
		}

		public NetHackApp.DialogResponse askUserInstallLocation()
		{
			setDialogResponse(NetHackApp.DialogResponse.Invalid);
			activityNetHackApp.handler.sendEmptyMessage(NetHackApp.MSG_SHOW_DIALOG_ASK_INSTALL_LOCATION);
			waitForResponse();
			return dialogResponse;
		}
		public NetHackApp.DialogResponse askUserIfReinstallOnInternalMemory()
		{
			setDialogResponse(NetHackApp.DialogResponse.Invalid);
			activityNetHackApp.handler.sendEmptyMessage(NetHackApp.MSG_SHOW_DIALOG_EXISTING_EXTERNAL_INSTALLATION_UNAVAILABLE);
			waitForResponse();
			return dialogResponse;
		}
		public NetHackApp.DialogResponse askUserIfInstallOnInternalMemory()
		{
			setDialogResponse(NetHackApp.DialogResponse.Invalid);
			activityNetHackApp.handler.sendEmptyMessage(NetHackApp.MSG_SHOW_DIALOG_SD_CARD_NOT_FOUND);
			waitForResponse();
			return dialogResponse;
		}
		public NetHackApp.DialogResponse askUserIfMoveOldInstallationToExternalMemory()
		{
			setDialogResponse(NetHackApp.DialogResponse.Invalid);
			activityNetHackApp.handler.sendEmptyMessage(NetHackApp.MSG_SHOW_DIALOG_MOVE_OLD_INSTALLATION);
			waitForResponse();
			return dialogResponse;
		}
		

		
		public int countAllFilesInDirectoryTree(File dirorfile)
		{
			if(dirorfile.isDirectory())
			{
				int cnt = 0;
				String[] children = dirorfile.list();
				for(int i = 0; i < children.length; i++)
				{
					cnt += countAllFilesInDirectoryTree(new File(dirorfile, children[i]));
				}

				// Count ourselves.
				cnt++;

				return cnt;
			}
			else
			{
				// Should we check if it exists here?
				return 1;
			}
		}

		public int countAllFilesInDirectoryTree(String dirname)
		{
			return countAllFilesInDirectoryTree(new File(dirname));
		}

		public boolean deleteAllFilesInDirectoryTree(String srcname, boolean reportprogress)
		{
			boolean success = true;

			File srcfile = new File(srcname);
			if(srcfile.isDirectory())
			{
				String[] children = srcfile.list();
				for(int i = 0; i < children.length; i++)
				{
					if(!deleteAllFilesInDirectoryTree(srcname + "/" + children[i], reportprogress))
					{
						// Even if we failed to delete something, we continue trying with the rest of
						// the files. Probably makes the most sense.
						success = false;
					}
				}

			}
			if(reportprogress)
			{
				reportProgress();
			}
			if(!srcfile.delete())
			{
				// Should we do this? Or just continue?
				return false;	
			}
			return success;
		}
 
		public boolean copyAllFilesInDirectoryTree(String srcname, String destname, boolean reportprogress)
		{
			File srcfile = new File(srcname);
			if(srcfile.isDirectory())
			{
				NetHackFileHelpers.mkdir(destname);

				String[] children = srcfile.list();
				for(int i = 0; i < children.length; i++)
				{
					if(!copyAllFilesInDirectoryTree(srcname + "/" + children[i], destname + "/" + children[i], reportprogress))
					{
						return false;
					}
				}
			}
			else
			{
				try
				{
					NetHackFileHelpers.copyFileRaw(srcname, destname);
				}
				catch(IOException e)
				{
					return false;	// Failed					
				}
			}
			if(reportprogress)
			{
				reportProgress();
			}
			return true;
		}

		public void moveOldInstallationToExternalMemory()
		{
			setAppDir(false);
			String srcdir = getNetHackDir();
			setAppDir(true);
			String destdir = getNetHackDir();
			int finalprogress = countAllFilesInDirectoryTree(srcdir);

			finalprogress *= 2;	// First copy, then delete.

			activityNetHackApp.handler.sendMessage(Message.obtain(activityNetHackApp.handler, NetHackApp.MSG_MOVE_INSTALL_BEGIN, finalprogress, 0, null));
	
			installProgress = 0;

			if(copyAllFilesInDirectoryTree(srcdir, destdir, true))
			{
				Log.i("NetHackDbg", "File copy succeeded!");
			}
			else
			{
				// TODO: Should probably notify the user or something in this case.
				Log.i("NetHackDbg", "File copy failed!");
				System.exit(0);
			}

			if(deleteAllFilesInDirectoryTree(srcdir, true))
			{
				Log.i("NetHackDbg", "Delete succeeded!");
			}
			else
			{
				// TODO: Should probably notify the user or something in this case, though there
				// shouldn't be a need to abort - can probably just leave the undeletable files around
				// without too much of a problem.
				Log.i("NetHackDbg", "Delete failed!");				
			}

			activityNetHackApp.handler.sendEmptyMessage(NetHackApp.MSG_MOVE_INSTALL_END);

			storePrefsInstalledOnExternalMemory(true);

			// If we were in the obsolete path before, we have moved from there.
			storePrefsInstalledInObsoletePath(false);
			useObsoletePath = false;
		}

		public boolean checkForOlderVersion(boolean checkobsoletelocation)
		{
			useObsoletePath = checkobsoletelocation;

			setAppDir(false);

			String destname = getAppDir() + "/" + "version.txt";
			boolean found = new File(destname).exists();

			if(!found)
			{
				// Didn't find an old 'version.txt' file amongst the internal files, doesn't look
				// like we're upgrading from an older version.
				return true;	// Continue
			}

			if(isExistingInstallationUpToDate())
			{
				// Can't think of why this would happen - if we have an up to date installation,
				// checkForOlderVersion() probably shouldn't be called in the first place.
				return true;	// Continue
			}

			if(NetHackFileHelpers.checkExternalStorageReady())
			{
				NetHackApp.DialogResponse r = askUserIfMoveOldInstallationToExternalMemory();
				if(r == NetHackApp.DialogResponse.Yes)
				{
					moveOldInstallationToExternalMemory();
				}
				else if(r == NetHackApp.DialogResponse.No)
				{
					// User didn't want to move. Remember that the current installation is in internal
					// memory. It should be updated to the new version soon, preserving existing saved games.
					storePrefsInstalledOnExternalMemory(false);

					// Perhaps we're left in the obsolete path then?
					storePrefsInstalledInObsoletePath(useObsoletePath);
				}
				else	// NetHackApp.DialogResponse.Exit
				{
					return false;	// Exit
				}
				
			}
			else
			{
				// In this case, the SD card is unavailable, and there is data from the old version in
				// internal memory. Since we can't move it to the SD card, just remember that we're
				// using internal memory for file storage. We could notify the user here, but I expect
				// this to be a rare situation, and going away as we move to future upgrades.
				storePrefsInstalledOnExternalMemory(false);

				// Perhaps we're left in the obsolete path then?
				storePrefsInstalledInObsoletePath(useObsoletePath);
			}
			return true;
		}

		public boolean performInitialChecks()
		{
			useObsoletePath = readPrefsInstalledInObsoletePath();

			if(!doesExistingInstallationExist())
			{
				if(!checkForOlderVersion(false))
				{
					// Quit - should only happen if the user said so in a dialog.
					return false;
				}

				// At this point, we may have found an old installation and moved it to the SD card,
				// or found an old installation and left it in place, or we didn't find an old installation
				// in the place where we looked. If we still haven't found anything, also check the
				// pre-1.2.2 file location:
				if(!doesExistingInstallationExist())
				{
					// Test for the old path, which was used up until version 1.2.1. Even if people have upgraded
					// since then, the previous versions I believe have all supported leaving the files installed
					// there, so we have to be careful to not ruin somebody's old data files now.
					if(!checkForOlderVersion(true))
					{
						// Quit - should only happen if the user said so in a dialog.
						return false;
					}

					// If we still didn't find any existing installation, make sure we don't try
					// to install the new version in the obsolete path!
					if(!doesExistingInstallationExist())
					{
						useObsoletePath = false;	
					}
				}
			}
			
			// Check preferences to see if there is an existing installation.
			if(doesExistingInstallationExist())
			{
				// If there is an existing installation, check if it's available.
				while(!isExistingInstallationAvailable())
				{
					NetHackApp.DialogResponse r = askUserIfReinstallOnInternalMemory();
					if(r == NetHackApp.DialogResponse.Yes)
					{
						install(false);
					}
					else if(r == NetHackApp.DialogResponse.No)
					{
						return false;
					}
				}
				
				SharedPreferences prefs = PreferenceManager.getDefaultSharedPreferences(activityNetHackApp.getBaseContext());
				boolean existinginstallationexternal = prefs.getBoolean("InstalledOnExternalMemory", false);

				setAppDir(existinginstallationexternal);

				// If available, check if up to date.
				if(isExistingInstallationUpToDate())
				{
					// If up to date, start.
					return true;
				}
				else
				{
					install(existinginstallationexternal);
					return true;
				}
			}
			else
			{
				boolean installexternally = true;

				if(NetHackFileHelpers.checkExternalStorageReady())
				{
					// The SD card appears to be ready in normal order, but it's probably still
					// best to ask. The main thought behind this is that if some users have problems
					// using the SD card, they have the choice to install to internal memory.
					NetHackApp.DialogResponse r = askUserInstallLocation();
					if(r == NetHackApp.DialogResponse.Yes)
					{
						installexternally = true;
					}
					else if(r == NetHackApp.DialogResponse.No)
					{
						installexternally = false;
					}
					else if(r == NetHackApp.DialogResponse.Exit)
					{
						return false;
					}
				}

				while(installexternally && !NetHackFileHelpers.checkExternalStorageReady())
				{
					NetHackApp.DialogResponse r = askUserIfInstallOnInternalMemory();
					if(r == NetHackApp.DialogResponse.Yes)
					{
						installexternally = false;
					}
					else if(r == NetHackApp.DialogResponse.No)
					{
						return false;
					}
				}
				install(installexternally);
				return true;
			}
		}
		public void run()
		{
			if(performInitialChecks())
			{
				activityNetHackApp.handler.sendEmptyMessage(NetHackApp.MSG_LAUNCH_GAME);
			}
			else
			{
				activityNetHackApp.handler.sendEmptyMessage(NetHackApp.MSG_QUIT);
			}
		}
	};
}
