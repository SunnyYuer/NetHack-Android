package com.nethackff;

public class NetHackJNI
{
	public native int NetHackInit(int puretty, String nethackdir);
	public native void NetHackShutdown();
	public native String NetHackTerminalReceive();
	public native void NetHackTerminalSend(String str);
	public native void NetHackSendDir(int moveDir, int allowcontext);	// enum MoveDir
	public native void NetHackMapTap(int x, int y);
	public native int NetHackHasQuit();
	public native int NetHackSave();
	public native void NetHackSetScreenDim(int msgwidth, int nummsglines, int statuswidth);
	public native void NetHackRefreshDisplay();
	public native void NetHackSwitchCharSet(int charsetindex);
	public native void NetHackSetTilesEnabled(int tilesenabled);
	
	public native int NetHackGetPlayerPosX();
	public native int NetHackGetPlayerPosY();
	public native int NetHackGetPlayerPosShouldRecenter();
	
	static
	{
		System.loadLibrary("nethack");
	}
}
