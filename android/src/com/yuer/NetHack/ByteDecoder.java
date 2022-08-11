package com.yuer.NetHack;

public interface ByteDecoder
{
	char decode(int b);
	String decode(byte[] bytes);
}
