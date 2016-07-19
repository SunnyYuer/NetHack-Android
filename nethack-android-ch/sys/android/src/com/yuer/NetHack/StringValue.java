package com.yuer.NetHack;

import java.io.BufferedReader;
import java.io.File;
import java.io.FileInputStream;
import java.io.InputStream;
import java.io.InputStreamReader;
import java.util.HashMap;
import java.util.Map;

import android.content.Context;

import com.yuer.NetHack.NetHack;

public class StringValue {
	public static StringValue inst = new StringValue();
	
	private Context context;
	
	public StringValue(){}
	private void _Init(NetHack nethack){
		context = nethack;
	}
	
	public static void Init(NetHack nethack){
		inst._Init(nethack);
	}
	
	public static String getKey(int res){
		return inst.context.getString(res);		
	}
}
