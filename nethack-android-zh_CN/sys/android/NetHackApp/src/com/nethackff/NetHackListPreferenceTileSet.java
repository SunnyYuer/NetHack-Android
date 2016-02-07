package com.nethackff;

import android.content.Context;
import android.preference.ListPreference;
import android.util.AttributeSet;

public class NetHackListPreferenceTileSet extends ListPreference
{
	public NetHackListPreferenceTileSet(Context context)
	{
		super(context);
	}

	public NetHackListPreferenceTileSet(Context context, AttributeSet attrs)
	{
		super(context, attrs);
	}

	public void setTileSetInfo(String tilesetinfo[])
	{
		tileSetInfo = tilesetinfo;
	}

	public void setInfoFromValue()
	{
		String info = "";
		int index = findIndexOfValue(getValue());

		if(index < 0)
		{
			// Doing this seems consistent with the behavior we would get from
			// NetHackApp.usePreferredTileSet(), picking the first from the list
			// if the current selection doesn't seem to match any of the options.
			index = 0;
		}

		if(index < tileSetInfo.length)
		{
			info = tileSetInfo[index];
		}
		setSummary(info);
	}

	String tileSetInfo[];

	protected void onDialogClosed(boolean positiveResult)
	{
		super.onDialogClosed(positiveResult);

		setInfoFromValue();
	}
};
