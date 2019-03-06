package com.yuer.NetHack;

import java.io.UnsupportedEncodingException;

public class Chinese {
	/*
	 * Yuer
	 */
	
	public String encode(byte[] s)
	{
		int i, j = 0;
		int l = s.length;
		for(i = 0; i <= l-2; i++) {
			if (s[i] == 32 && s[i + 1] < 0)
			{
				j++;
				i++;
			}
		}
		byte[] st = new byte[l-j];
		j = 0;
		for(i = 0; i <= l-2; i++) {
			if (s[i] == 32 && s[i + 1] < 0) i++;
			//System.out.println(" "+i+' '+s[i]);
			st[j] = s[i];
			j++;
		}
		if(l > 0) st[j] = s[i];
		String cn = "";
		try {
			cn = new String(st, "UTF-8");
			//System.out.println(cn);
		} catch (UnsupportedEncodingException e) {
			// TODO Auto-generated catch block
			e.printStackTrace();
		}
		return cn;
	}
	
	public byte[] encode(String s)
	{
		return s.getBytes();
	}
}
