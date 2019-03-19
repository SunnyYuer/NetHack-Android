package com.yuer.NetHack;

import java.io.UnsupportedEncodingException;

public class Chinese {
	/*
	 * Yuer
	 */
	
	public String encode(byte[] s)
	{
		int spe = 0;
		char spech = ' ';
		int i, j = 0;
		int l = s.length;
		for(i = 0; i <= l-2; i++) {
			if (s[i] == 32 && s[i + 1] < 0)
			{
				j++;
				i++;
			}
			if(i==0 && s[0]<0 && s[1]>0)
			{
				spe = 1;
				ByteDecoder decoder = new CP437();
				spech = decoder.decode(s[0]);
				//System.out.println(decoder.decode(s[0])+""+(int)decoder.decode(s[0]));
			}
		}
		byte[] st = new byte[l-j-spe];
		j = 0;
		for(i = spe; i <= l-2; i++) {
			if (s[i] == 32 && s[i + 1] < 0) i++;
			//System.out.println(" "+i+' '+s[i]);
			st[j] = s[i];
			j++;
		}
		if(l > 0) st[j] = s[i];
		String cn = "";
		try {
			cn = new String(st, "UTF-8");
			if(spe == 1) cn = spech + cn;
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
