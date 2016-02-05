package com.nethackff;

import java.util.Map;

import android.app.AlertDialog;
import android.app.Dialog;
import android.app.ListActivity;
import android.content.Context;
import android.content.DialogInterface;
import android.content.DialogInterface.OnClickListener;
import android.content.SharedPreferences.OnSharedPreferenceChangeListener;
import android.content.SharedPreferences;
import android.os.Bundle;
import android.text.Editable;
import android.text.TextWatcher;
import android.view.KeyEvent;
import android.view.LayoutInflater;
import android.view.Menu;
import android.view.MenuItem;
import android.view.View;
import android.view.ViewGroup;
import android.widget.ArrayAdapter;
import android.widget.BaseAdapter;
import android.widget.Button;
import android.widget.EditText;
import android.widget.ListView;
import android.widget.TextView;
import android.widget.Toast;
import android.widget.TwoLineListItem;

public class KeyBindingListActivity extends ListActivity
{
	private static final String CURRENTLY_SELECTED_KEYCODE = "CURRENTLY_SELECTED_KEYCODE";
	private SharedPreferences keyMapPrefs;
	private Map<String, ?> keyMappings;
	private KeyBindingListAdapter adapter;
	private LayoutInflater inflater;
	private int currentlySelectedKeyCode = -1;
	private OnClickListener cancelOnClickListener = new OnClickListener()
	{

		@Override
		public void onClick(DialogInterface dialog, int which)
		{
			currentlySelectedKeyCode = -1;
		}
	};

	private enum KeyBindDialogType
	{
		GET_KEY,
		GET_KEY_BINDING_ACTION,
		GET_KEY_BINDING_NETHACK_KEY,
		SHOW_KEY_BINDING_OPTIONS;
	}

	@Override
	public void onCreate(Bundle savedInstanceState) 
	{
		super.onCreate(savedInstanceState);

		inflater = (LayoutInflater) getSystemService(Context.LAYOUT_INFLATER_SERVICE);
		View headerView = inflater.inflate(R.layout.key_binding_list_header, null);
		headerView.setOnClickListener(new View.OnClickListener()
		{
			
			@Override
			public void onClick(View v)
			{
				showDialog(KeyBindDialogType.GET_KEY.ordinal());
			}
		});
		getListView().addHeaderView(headerView);

		keyMapPrefs = getSharedPreferences(getString(R.string.keyMapPreferences), Context.MODE_PRIVATE);

		adapter = new KeyBindingListAdapter(this);
		setListAdapter(adapter);
		
	}

	@Override
	protected void onListItemClick (ListView l, View v, int position, long id)
	{
		currentlySelectedKeyCode = Integer.parseInt((String) getListView().getItemAtPosition(position));
		showDialog(KeyBindDialogType.SHOW_KEY_BINDING_OPTIONS.ordinal());
	}

	public void onAddKeyBindingClick(View v)
	{
		showDialog(KeyBindDialogType.GET_KEY.ordinal());
	}

	@Override
	protected Dialog onCreateDialog (int id)
	{
		KeyBindDialogType dialogType = KeyBindDialogType.values()[id];
		if (dialogType == KeyBindDialogType.GET_KEY)
		{
			return createGetKeyDialog();
		}
		else if (dialogType == KeyBindDialogType.GET_KEY_BINDING_ACTION)
		{
			return createGetActionDialog();
		}
		else if (dialogType == KeyBindDialogType.GET_KEY_BINDING_NETHACK_KEY)
		{
			return createCharacterBindingDialog();
		}
		else if (dialogType == KeyBindDialogType.SHOW_KEY_BINDING_OPTIONS)
		{
			return createKeyBindingOptionsDialog();
		}
		return null;
	}

	private Dialog createKeyBindingOptionsDialog()
	{
		AlertDialog.Builder builder = new AlertDialog.Builder(this);
		builder.setTitle(getKeyIsBoundToValueString());
		builder.setPositiveButton(R.string.dialog_key_binding_change, new OnClickListener()
		{

			@Override
			public void onClick(DialogInterface dialog, int which)
			{
				showDialog(KeyBindDialogType.GET_KEY.ordinal());
			}
		});
		builder.setNeutralButton(R.string.dialog_key_binding_delete, new OnClickListener()
		{

			@Override
			public void onClick(DialogInterface dialog, int which)
			{
				SharedPreferences.Editor editor = keyMapPrefs.edit();
				editor.remove(Integer.toString(currentlySelectedKeyCode));
				editor.commit();
				currentlySelectedKeyCode = -1;
			}
		});
		builder.setNegativeButton(R.string.dialog_Cancel, cancelOnClickListener);
		return builder.create();
	}

	private Dialog createCharacterBindingDialog()
	{
		final Dialog characterBindingDialog = new Dialog(this);
		characterBindingDialog.setContentView(R.layout.character_binding_dialog);
		characterBindingDialog.setTitle(getBindingKeyToCharacterString());
		final EditText characterField = (EditText) characterBindingDialog.findViewById(R.id.character_field);
		characterField.addTextChangedListener(new TextWatcher()
		{
			@Override
			public void onTextChanged(CharSequence s, int start, int before, int count)
			{
				//Not needed
			}
			
			@Override
			public void beforeTextChanged(CharSequence s, int start, int count, int after)
			{
				//Not needed
			}
			
			@Override
			public void afterTextChanged(Editable s)
			{
				characterField.selectAll();
			}
		});
		Button positiveButton = (Button) characterBindingDialog.findViewById(R.id.positiveButton);
		positiveButton.setOnClickListener(new View.OnClickListener()
		{
			@Override
			public void onClick(View v)
			{
				if (characterField.getText().length() == 0)
				{
					Toast.makeText(KeyBindingListActivity.this, R.string.please_select_a_character, Toast.LENGTH_SHORT).show();
					return;
				}

				Editable bindChar = characterField.getText();
				bindKeyCode(bindChar.toString());
				characterBindingDialog.dismiss();
			}
		});
		Button negativeButton = (Button) characterBindingDialog.findViewById(R.id.negativeButton);
		negativeButton.setOnClickListener(new View.OnClickListener()
		{
			@Override
			public void onClick(View v)
			{
				currentlySelectedKeyCode = -1;
				characterBindingDialog.dismiss();
			}
		});
		

		return characterBindingDialog;
	}

	private Dialog createGetActionDialog()
	{
		AlertDialog.Builder builder = new AlertDialog.Builder(this);
		builder.setAdapter(new ArrayAdapter<KeyAction>(getBaseContext(), R.layout.simple_list_item_black, KeyAction.values()), new OnClickListener()
		{
			public void onClick(DialogInterface dialog, int which)
			{
				String keyActionName = KeyAction.values()[which].name();
				bindKeyCode(keyActionName);
			}
		});
		builder.setNegativeButton(R.string.dialog_Cancel, cancelOnClickListener);
		builder.setTitle(getBindingKeyToActionString());
		return builder.create();
	}

	private Dialog createGetKeyDialog()
	{
		final Dialog getKeyDialog = new Dialog(this);
		getKeyDialog.setContentView(R.layout.get_key_dialog);
		getKeyDialog.setTitle(R.string.dialog_get_key_title);
		Button positiveButton = (Button) getKeyDialog.findViewById(R.id.positiveButton);
		positiveButton.setOnClickListener(new View.OnClickListener()
		{
			@Override
			public void onClick(View view)
			{
				if (!validKeySelected(getKeyDialog))
				{
					return;
				}
				showDialog(KeyBindDialogType.GET_KEY_BINDING_NETHACK_KEY.ordinal());
				getKeyDialog.dismiss();
			}
		});
		
		Button neutralButton = (Button) getKeyDialog.findViewById(R.id.neutralButton);
		neutralButton.setOnClickListener(new View.OnClickListener()
		{
			@Override
			public void onClick(View v)
			{
				if (!validKeySelected(getKeyDialog))
				{
					return;
				}
				showDialog(KeyBindDialogType.GET_KEY_BINDING_ACTION.ordinal());
				getKeyDialog.dismiss();
			}
		});
		
		Button negativeButton = (Button) getKeyDialog.findViewById(R.id.negativeButton);
		negativeButton.setOnClickListener(new View.OnClickListener()
		{
			@Override
			public void onClick(View v)
			{
				currentlySelectedKeyCode = -1;
				getKeyDialog.dismiss();
			}
		});
		getKeyDialog.setOnKeyListener(new Dialog.OnKeyListener()
		{
			public boolean onKey(DialogInterface dialogInterface, int keyCode, KeyEvent event)
			{
				TextView message = (TextView) getKeyDialog.findViewById(android.R.id.message);
				message.setText(KeyCodeSymbolicNames.keyCodeToString(keyCode));

				currentlySelectedKeyCode = keyCode;
				return true;
			}
		});
		return getKeyDialog;
	}
	private boolean validKeySelected(final Dialog getKeyDialog)
	{
		if (currentlySelectedKeyCode == -1)
		{
			Toast.makeText(KeyBindingListActivity.this, R.string.please_select_a_key, Toast.LENGTH_SHORT).show();
			return false;
		}
		if (currentlySelectedKeyCode == KeyEvent.KEYCODE_MENU)
		{
			Toast.makeText(KeyBindingListActivity.this,R.string.cant_bind_menu_key , Toast.LENGTH_SHORT).show();

			currentlySelectedKeyCode = -1;
			TextView message = (TextView) getKeyDialog.findViewById(android.R.id.message);
			message.setText(R.string.dialog_get_key_default_message);

			return false;
		}
		return true;
	}

	@Override
	protected void onPrepareDialog(int id, Dialog dialog)
	{
		if (id == KeyBindDialogType.GET_KEY.ordinal())
		{
			TextView message = (TextView) dialog.findViewById(android.R.id.message);
			if (currentlySelectedKeyCode == -1)
			{
				message.setText(R.string.dialog_get_key_default_message);
				return;
			}
			message.setText(KeyCodeSymbolicNames.keyCodeToString(currentlySelectedKeyCode));
			return;
		}
		else if (id == KeyBindDialogType.GET_KEY_BINDING_ACTION.ordinal())
		{
			dialog.setTitle(getBindingKeyToActionString());
		}
		else if (id == KeyBindDialogType.GET_KEY_BINDING_NETHACK_KEY.ordinal())
		{
			dialog.setTitle(getBindingKeyToCharacterString());
		}
		else if (id == KeyBindDialogType.SHOW_KEY_BINDING_OPTIONS.ordinal())
		{
			dialog.setTitle(getKeyIsBoundToValueString());
		}

	}

	private String getBindingKeyToActionString()
	{
		return "Binding " + KeyCodeSymbolicNames.keyCodeToString(currentlySelectedKeyCode) + " to other key: ";
	}

	private String getKeyIsBoundToValueString()
	{
		return KeyCodeSymbolicNames.keyCodeToString(currentlySelectedKeyCode) + " is bound to: " + keyMappings.get(String.valueOf(currentlySelectedKeyCode));
	}

	private void bindKeyCode(String bindTarget)
	{
		SharedPreferences.Editor editor = keyMapPrefs.edit();
		editor.putString(Integer.toString(currentlySelectedKeyCode), bindTarget);
		editor.commit();

		Toast.makeText(this, getKeyIsBoundToValueString(), Toast.LENGTH_SHORT).show();
		currentlySelectedKeyCode = -1;
	}

	private String getBindingKeyToCharacterString()
	{
		return "Binding " + KeyCodeSymbolicNames.keyCodeToString(currentlySelectedKeyCode) + " to Nethack character: ";
	}

	private class KeyBindingListAdapter extends BaseAdapter implements OnSharedPreferenceChangeListener
	{
		Object[] keys;
		public KeyBindingListAdapter(Context context)
		{
			refreshKeyMappings();
			keyMapPrefs.registerOnSharedPreferenceChangeListener(this);
		}

		private void refreshKeyMappings()
		{
			keyMappings = keyMapPrefs.getAll();
			keys = keyMappings.keySet().toArray();
		}

		@Override
		public int getCount()
		{
			return keyMappings.size();
		}

		@Override
		public Object getItem(int position)
		{
			return keys[position];
		}

		@Override
		public long getItemId(int position)
		{
			return position;
		}

		@Override
		public View getView(int position, View convertView, ViewGroup parent)
		{
			TwoLineListItem listItem;
			if (convertView == null)
			{
				LayoutInflater inflater = (LayoutInflater) parent.getContext().getSystemService(Context.LAYOUT_INFLATER_SERVICE);
				listItem = (TwoLineListItem) inflater.inflate(android.R.layout.simple_list_item_2, null);
			}
			else
			{
				listItem = (TwoLineListItem) convertView;
			}
			int keyCode = Integer.parseInt((String) keys[position]);
			String keyText = KeyCodeSymbolicNames.keyCodeToString(keyCode);
			listItem.getText1().setText(keyText);

			String valueText = (String) keyMappings.get(keys[position]); 
			listItem.getText2().setText(valueText);
			return listItem;
		}


		@Override
		public void onSharedPreferenceChanged(SharedPreferences sharedPreferences, String key)
		{
			refreshKeyMappings();
			notifyDataSetChanged();
		}
	}


	@Override
	protected void onSaveInstanceState(Bundle outState)
	{
		outState.putInt(CURRENTLY_SELECTED_KEYCODE, currentlySelectedKeyCode);
		super.onSaveInstanceState(outState);
	}

	@Override
	protected void onRestoreInstanceState (Bundle savedInstanceState)
	{
		currentlySelectedKeyCode = savedInstanceState.getInt(CURRENTLY_SELECTED_KEYCODE, -1);
	}


	@Override
	public boolean onCreateOptionsMenu(Menu menu)
	{
		super.onCreateOptionsMenu(menu);
		getMenuInflater().inflate(R.menu.key_binding_menu, menu);
		return true;
	}

	@Override
	public boolean onOptionsItemSelected(MenuItem item)
	{
		switch(item.getItemId())
		{
		case (R.id.reset_to_defaults):
		{
			clearBindings();
			keyMapPrefs = getSharedPreferences(getString(R.string.keyMapPreferences), Context.MODE_PRIVATE);
			NetHackApp.resetKeyBindingsToDefaults(keyMapPrefs);
			return true;
		}
		case (R.id.clear_all_bindings):
		{
			clearBindings();
			adapter.onSharedPreferenceChanged(keyMapPrefs, null);
			return true;
		}
		default:
			return super.onOptionsItemSelected(item);
		}
	}

	private void clearBindings()
	{
		SharedPreferences keyMapPrefs = getSharedPreferences(getString(R.string.keyMapPreferences), Context.MODE_PRIVATE);
		SharedPreferences.Editor editor = keyMapPrefs.edit();
		editor.clear();
		editor.commit();
	}
}
