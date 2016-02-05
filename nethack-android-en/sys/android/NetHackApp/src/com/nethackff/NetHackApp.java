package com.nethackff;

import android.app.Activity;
import android.app.AlertDialog;
import android.app.Dialog;
import android.app.ProgressDialog;
import android.content.Context;
import android.content.DialogInterface;
import android.content.SharedPreferences;
import android.content.res.Configuration;
import android.os.Bundle;
import android.os.Handler;
import android.os.Message;
import android.util.Log;
import android.view.KeyEvent;
import android.view.Menu;
import android.view.MenuItem;
import android.view.MotionEvent;
import android.view.Window;



public class NetHackApp extends Activity
{
	public NetHackGame game;

	static final int MSG_SHOW_DIALOG_SD_CARD_NOT_FOUND = 100;	// TEMP
	static final int MSG_INSTALL_BEGIN = 101;
	static final int MSG_INSTALL_END = 102;
	static final int MSG_INSTALL_PROGRESS = 103;
	static final int MSG_MOVE_INSTALL_BEGIN = 104;
	static final int MSG_MOVE_INSTALL_END = 105;
	static final int MSG_MOVE_INSTALL_PROGRESS = 106;
	static final int MSG_SHOW_DIALOG_ASK_INSTALL_LOCATION = 107;
	static final int MSG_SHOW_DIALOG_EXISTING_EXTERNAL_INSTALLATION_UNAVAILABLE = 108;
	static final int MSG_SHOW_DIALOG_MOVE_OLD_INSTALLATION = 109;
	static final int MSG_LAUNCH_GAME = 110;
	static final int MSG_QUIT = 111;

	static final int DIALOG_SD_CARD_NOT_FOUND = 0;
	static final int DIALOG_ASK_INSTALL_LOCATION = 1;
	static final int DIALOG_EXISTING_EXTERNAL_INSTALLATION_UNAVAILABLE = 2;
	static final int DIALOG_MOVE_OLD_INSTALLATION = 3;
	static final int DIALOG_INSTALL_PROGRESS = 4;
	static final int DIALOG_MOVE_INSTALL_PROGRESS = 5;
	
	private ProgressDialog progressDialog = null;

	// TODO: Protect?
	public Handler handler = new Handler()
	{
		public void handleMessage(Message msg)
		{
			switch(msg.what)
			{
				case MSG_SHOW_DIALOG_SD_CARD_NOT_FOUND:
					NetHackApp.this.showDialog(DIALOG_SD_CARD_NOT_FOUND);
					break;
				case MSG_SHOW_DIALOG_ASK_INSTALL_LOCATION:
					NetHackApp.this.showDialog(DIALOG_ASK_INSTALL_LOCATION);
					break;
				case MSG_SHOW_DIALOG_EXISTING_EXTERNAL_INSTALLATION_UNAVAILABLE:
					NetHackApp.this.showDialog(DIALOG_EXISTING_EXTERNAL_INSTALLATION_UNAVAILABLE);
					break;
				case MSG_SHOW_DIALOG_MOVE_OLD_INSTALLATION:
					NetHackApp.this.showDialog(DIALOG_MOVE_OLD_INSTALLATION);
					break;
				case MSG_INSTALL_BEGIN:
					NetHackApp.this.showDialog(DIALOG_INSTALL_PROGRESS);
					if(progressDialog != null)
					{
						progressDialog.setMax(msg.arg1);
					}
					break;
				case MSG_INSTALL_END:
					progressDialog = null;
					NetHackApp.this.dismissDialog(DIALOG_INSTALL_PROGRESS);
					break;
				case MSG_INSTALL_PROGRESS:
				case MSG_MOVE_INSTALL_PROGRESS:
					if(progressDialog != null)
					{
						progressDialog.setProgress(msg.arg1);
					}
					break;
				case MSG_MOVE_INSTALL_BEGIN:
					NetHackApp.this.showDialog(DIALOG_MOVE_INSTALL_PROGRESS);
					if(progressDialog != null)
					{
						progressDialog.setMax(msg.arg1);
					}
					break;
				case MSG_MOVE_INSTALL_END:
					progressDialog = null;
					NetHackApp.this.dismissDialog(DIALOG_MOVE_INSTALL_PROGRESS);
					break;
				case MSG_LAUNCH_GAME:
					// TODO: Generate exception if game already exists somehow?
					game = new NetHackGame(NetHackApp.this);
					game.onCreate();
					game.onStart();
					game.onResume();
//					Intent intent = new Intent(NetHackApp.this, NetHackGameActivity.class);
//					Bundle bundle = new Bundle();
//					intent.putExtras(bundle);
//					startActivity(intent);
					break;
				case MSG_QUIT:
					// Is this the way to do it? TODO: Make sure thread exits cleanly and stuff.
					finish();
					break;
				default:
					// What?
					break;
			}
		}
	};

	String appDir;

	NetHackInstaller installer;
	

	public String getAppDir()
	{
		return appDir;
	}

	enum DialogResponse
	{
		Invalid,
		Yes,
		No,
		Retry,
		Exit
	};

	protected Dialog onCreateDialog(int id)
	{
	    Dialog dialog;
	    switch(id)
	    {
		    case DIALOG_SD_CARD_NOT_FOUND:
		    {
				AlertDialog.Builder builder = new AlertDialog.Builder(this);  
				builder	.setMessage(getString(R.string.dialog_msg_sd_card_not_found))
						.setCancelable(false)
						.setPositiveButton(getString(R.string.dialog_button_retry), new DialogInterface.OnClickListener()
						{
							public void onClick(DialogInterface dialog, int id)
							{
								installer.installThread.setDialogResponse(DialogResponse.Retry);
							}
						})
						.setNeutralButton(getString(R.string.dialog_button_use_internal_memory), new DialogInterface.OnClickListener()
						{
							public void onClick(DialogInterface dialog, int id)
							{
								installer.installThread.setDialogResponse(DialogResponse.Yes);
							}
						})
						.setNegativeButton(getString(R.string.dialog_button_exit), new DialogInterface.OnClickListener()
						{
							public void onClick(DialogInterface dialog, int id)
							{
								installer.installThread.setDialogResponse(DialogResponse.No);
								//NetHackApp.this.finish();
							}
						});
				dialog = builder.create();
				//alert.show();
				break;
	    	}
		    case DIALOG_ASK_INSTALL_LOCATION:
		    {
				AlertDialog.Builder builder = new AlertDialog.Builder(this);  
				builder	.setMessage(getString(R.string.dialog_msg_ask_install_location))
						.setCancelable(false)
						.setPositiveButton(getString(R.string.dialog_button_install_on_sd_card), new DialogInterface.OnClickListener()
						{
							public void onClick(DialogInterface dialog, int id)
							{
								installer.installThread.setDialogResponse(DialogResponse.Yes);
							}
						})
						.setNeutralButton(getString(R.string.dialog_button_install_on_internal_memory), new DialogInterface.OnClickListener()
						{
							public void onClick(DialogInterface dialog, int id)
							{
								installer.installThread.setDialogResponse(DialogResponse.No);
								//dialog.cancel();
							}
						})
						.setNegativeButton(getString(R.string.dialog_button_exit), new DialogInterface.OnClickListener()
						{
							public void onClick(DialogInterface dialog, int id)
							{
								installer.installThread.setDialogResponse(DialogResponse.Exit);
								//NetHackApp.this.finish();
							}
						});
				dialog = builder.create();
				break;
		    }
		    case DIALOG_EXISTING_EXTERNAL_INSTALLATION_UNAVAILABLE:
		    {
				AlertDialog.Builder builder = new AlertDialog.Builder(this);  
				builder	.setMessage(getString(R.string.dialog_msg_existing_external_installation_unavailable))
						.setCancelable(false)
						.setPositiveButton(getString(R.string.dialog_button_retry), new DialogInterface.OnClickListener()
						{
							public void onClick(DialogInterface dialog, int id)
							{
								installer.installThread.setDialogResponse(DialogResponse.Retry);
							}
						})
						.setNeutralButton(getString(R.string.dialog_button_reinstall_on_internal_memory), new DialogInterface.OnClickListener()
						{
							public void onClick(DialogInterface dialog, int id)
							{
								installer.installThread.setDialogResponse(DialogResponse.Yes);
								//dialog.cancel();
							}
						})
						.setNegativeButton(getString(R.string.dialog_button_exit), new DialogInterface.OnClickListener()
						{
							public void onClick(DialogInterface dialog, int id)
							{
								installer.installThread.setDialogResponse(DialogResponse.No);
								//NetHackApp.this.finish();
							}
						});
				dialog = builder.create();
				//alert.show();
				break;
		    }
		    case DIALOG_MOVE_OLD_INSTALLATION:
		    {
				AlertDialog.Builder builder = new AlertDialog.Builder(this);  
				builder	.setMessage(getString(R.string.dialog_msg_move_old_installation))
						.setCancelable(false)
						.setPositiveButton(getString(R.string.dialog_button_move_to_sd_card), new DialogInterface.OnClickListener()
						{
							public void onClick(DialogInterface dialog, int id)
							{
								installer.installThread.setDialogResponse(DialogResponse.Yes);
							}
						})
						.setNeutralButton(getString(R.string.dialog_button_keep_existing_installation), new DialogInterface.OnClickListener()
						{
							public void onClick(DialogInterface dialog, int id)
							{
								installer.installThread.setDialogResponse(DialogResponse.No);
								//dialog.cancel();
							}
						})
						.setNegativeButton(getString(R.string.dialog_button_exit), new DialogInterface.OnClickListener()
						{
							public void onClick(DialogInterface dialog, int id)
							{
								installer.installThread.setDialogResponse(DialogResponse.Exit);
								//NetHackApp.this.finish();
							}
						});
				dialog = builder.create();
				break;
		    }
		    case DIALOG_INSTALL_PROGRESS:
				progressDialog = new ProgressDialog(this);
				progressDialog.setProgressStyle(ProgressDialog.STYLE_HORIZONTAL);
				progressDialog.setMessage(getString(R.string.dialog_msg_install_progress) + " " + appDir);
				progressDialog.setCancelable(false);
				dialog = progressDialog;
				break;
		    case DIALOG_MOVE_INSTALL_PROGRESS:
				progressDialog = new ProgressDialog(this);
				progressDialog.setProgressStyle(ProgressDialog.STYLE_HORIZONTAL);
				progressDialog.setMessage(getString(R.string.dialog_msg_move_install_progress) + " " + appDir);
				progressDialog.setCancelable(false);
				dialog = progressDialog;
				break;
		    default:
		        dialog = null;
		    	break;
	    }
	    return dialog;
    }


	public void onCreate(Bundle savedInstanceState)
	{
		super.onCreate(savedInstanceState);

		requestWindowFeature(Window.FEATURE_NO_TITLE);

		// TODO: Make sure we remove the actual XML data for this.
		//setContentView(R.layout.install);
		setDefaultPreferences();

		installer = new NetHackInstaller(this.getAssets(), this, true);
	}


	private void setDefaultPreferences()
	{
		SharedPreferences keyMapPrefs = getSharedPreferences(getString(R.string.keyMapPreferences), Context.MODE_PRIVATE);
		if(keyMapPrefs.getAll().isEmpty())
		{
			resetKeyBindingsToDefaults(keyMapPrefs);
		}
	}


	public static void resetKeyBindingsToDefaults(SharedPreferences keyMapPrefs)
	{
		SharedPreferences.Editor editor = keyMapPrefs.edit();
		
		editor.putString(Integer.toString(KeyEvent.KEYCODE_BACK), KeyAction.ForwardToSystem.name());
		editor.putString(Integer.toString(KeyEvent.KEYCODE_CAMERA), KeyAction.VirtualKeyboard.name());
		editor.putString(Integer.toString(KeyEvent.KEYCODE_SEARCH), KeyAction.CtrlKey.name());
		editor.putString(Integer.toString(KeyEvent.KEYCODE_VOLUME_UP), KeyAction.ZoomIn.name());
		editor.putString(Integer.toString(KeyEvent.KEYCODE_VOLUME_DOWN), KeyAction.ZoomOut.name());
		editor.putString(Integer.toString(KeyEvent.KEYCODE_ALT_LEFT), KeyAction.AltKey.name());
		editor.putString(Integer.toString(KeyEvent.KEYCODE_SHIFT_LEFT), KeyAction.ShiftKey.name());
		editor.putString(Integer.toString(KeyEvent.KEYCODE_SHIFT_RIGHT), KeyAction.ShiftKey.name());

		editor.commit();
	}

	public void onConfigurationChanged(Configuration newConfig)
	{
		super.onConfigurationChanged(newConfig);

		if(game != null)
		{
			game.onConfigurationChanged(newConfig);
		}
	}

	public boolean onCreateOptionsMenu(Menu menu)
	{
		super.onCreateOptionsMenu(menu);
		getMenuInflater().inflate(R.menu.menu, menu);
		return true;
	}

	public void onDestroy()
	{
		if(game != null)
		{
			game.stopCommThread();
		}
		super.onDestroy();
		//TestShutdown();
	}

	public boolean onKeyDown(int keycode, KeyEvent event)
	{
		if(game != null)
		{
			return game.onKeyDown(keycode, event); 
		}
		else
		{
			return super.onKeyDown(keycode, event);	
		}
	}

	public boolean onKeyDownSuper(int keycode, KeyEvent event)
	{
		return super.onKeyDown(keycode, event);	
	}

	public boolean onKeyUp(int keycode, KeyEvent event)
	{
		if(game != null)
		{
			return game.onKeyUp(keycode, event); 
		}
		else
		{
			return super.onKeyUp(keycode, event);	
		}
	}

	public boolean onKeyUpSuper(int keycode, KeyEvent event)
	{
		return super.onKeyUp(keycode, event);	
	}

	public void onStart()
	{
		super.onStart();

		if(game != null)
		{
			game.onStart();
		}
	}

	public void onPause()
	{
		if(game != null)
		{
			if(game.jni.NetHackHasQuit() == 0)
			{
				Log.i("NetHack", "Auto-saving");
				if(game.jni.NetHackSave() != 0)
				{
					Log.i("NetHack", "Auto-save succeeded");
				}
				else
				{
					Log.w("NetHack", "Auto-save failed");
				}
			}		
			game.stopCommThread();
		}

		super.onPause();
	}

	public void onResume()
	{
		super.onResume();

		if(game != null)
		{
			game.onResume();
		}
	}

	public boolean onTouchEvent(MotionEvent me)
	{
		if(game != null)
		{
			return game.onTouchEvent(me);
		}
		else
		{
			return super.onTouchEvent(me);	
		}
	}

	public boolean onOptionsItemSelected(MenuItem item)
	{
		if(game != null)
		{
			return game.onOptionsItemSelected(item);
		}
		else
		{
			return super.onOptionsItemSelected(item);
		}
	}

}
