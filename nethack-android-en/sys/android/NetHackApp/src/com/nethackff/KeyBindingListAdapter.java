//package com.nethackff;
//
//import java.util.Map;
//
//import android.content.Context;
//import android.content.SharedPreferences;
//import android.view.KeyCharacterMap;
//import android.view.KeyEvent;
//import android.view.LayoutInflater;
//import android.view.View;
//import android.view.ViewGroup;
//import android.widget.BaseAdapter;
//import android.widget.TwoLineListItem;
//
//public class KeyBindingListAdapter extends BaseAdapter
//{
//	final public SharedPreferences keyMapPrefs;
//	public Map<String, ?> keyMappings;
//	Object[] keys;
//	public KeyBindingListAdapter(Context context)
//	{
//		keyMapPrefs = context.getSharedPreferences(context.getString(R.string.keyMapPreferences), Context.MODE_PRIVATE);
//		keyMappings = keyMapPrefs.getAll();
//		keys = keyMappings.keySet().toArray();
//	}
//
//	public int getCount()
//	{
//		return keyMappings.size();
//	}
//
//	public Object getItem(int position)
//	{
//		return keyMappings.get(keys[position]);
//	}
//
//	public long getItemId(int position)
//	{
//		return position;
//	}
//
//	public View getView(int position, View convertView, ViewGroup parent)
//	{
//		TwoLineListItem listItem;
//		if (convertView == null)
//		{
//			LayoutInflater inflater = (LayoutInflater) parent.getContext().getSystemService(Context.LAYOUT_INFLATER_SERVICE);
//			listItem = (TwoLineListItem) inflater.inflate(android.R.layout.simple_list_item_2, null);
//		}
//		else
//		{
//			listItem = (TwoLineListItem) convertView;
//		}
//		int keyCode = Integer.parseInt((String) keys[position]);
//		String keyText = KeyCodeSymbolicNames.keyCodeToString(keyCode);
//		listItem.getText1().setText(keyText);
//		
//		String valueText = (String) keyMappings.get(keys[position]); 
//		listItem.getText2().setText(valueText);
//		return listItem;
//	}
//}