package com.nethackff;

public enum KeyAction
{
	None,
	VirtualKeyboard,
	AltKey,
	CtrlKey,
	ShiftKey,
	EscKey,
	ZoomIn,
	ZoomOut,
	ForwardToSystem;		// Forward for O/S to handle.
	
	@Override 
	public String toString() 
	{
		return name();
	}
}
