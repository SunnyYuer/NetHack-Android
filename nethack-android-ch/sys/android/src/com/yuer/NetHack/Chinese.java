package com.yuer.NetHack;

import java.io.UnsupportedEncodingException;

public class Chinese {
	/*
	 * Yuer
	 */
	
	public String encode(byte[] s)
	{
		int i=0;
		String st="";
		int l=s.length;
		for(i=0;i<=l-1;i++)
		{
			//System.out.println((char)s[i]);
			if(s[i]<0)
			{
				byte[] b={s[i],s[i+1],s[i+2]};
				try {
					String code=new String(b,"utf-8");
					//System.out.println(code);
					st=st+code;
					i=i+2;
				} catch (UnsupportedEncodingException e) {
					// TODO Auto-generated catch block
					e.printStackTrace();
				}
			}
			else
			{
				if(i!=l-1)
				{
					if(s[i]>=0&&s[i+1]<0) continue;
				}
				st=st+(char)s[i];
			}
		}
		return st;
	}
	
	public byte[] encode(String s)
	{
		return s.getBytes();
	}
}
