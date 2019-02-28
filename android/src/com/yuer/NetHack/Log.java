package com.yuer.NetHack;

public class Log
{
	// ____________________________________________________________________________________
	public static void print(String string)
	{
		if(DEBUG.isOn())
			android.util.Log.i("NetHack", string);
	}

	// ____________________________________________________________________________________
	public static void print(int i)
	{
		if(DEBUG.isOn())
			print(Integer.toString(i));
	}
}
