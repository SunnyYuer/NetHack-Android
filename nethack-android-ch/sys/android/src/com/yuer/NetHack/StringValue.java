package com.yuer.NetHack;

import java.io.BufferedReader;
import java.io.File;
import java.io.FileInputStream;
import java.io.InputStream;
import java.io.InputStreamReader;
import java.util.HashMap;
import java.util.Map;

import com.yuer.NetHack.NetHack;

public class StringValue {
	public static StringValue inst = new StringValue();
	
	private Map<String, String> values = new HashMap();
	
	public StringValue(){}
	private void _Init(NetHack nethack){
		try{
			InputStream fis = nethack.getResources().getAssets().open("string_cn.csv");
			InputStreamReader reader = new InputStreamReader(fis);
			
			BufferedReader br = new BufferedReader(reader);
			String txt ;
			while((txt = br.readLine()) != null){
				String[] arr = txt.split(",");
				values.put(arr[0], arr[1]);
			}
			reader.close();
		}
		catch(Exception e){
			
		}
	}
	
	public static void Init(NetHack nethack){
		inst._Init(nethack);
	}
	
	public static String getKey(String k){
		String v = (String)inst.values.get(k);
		if(v != null)
			return v;
		else
			return k;			
	}
}
