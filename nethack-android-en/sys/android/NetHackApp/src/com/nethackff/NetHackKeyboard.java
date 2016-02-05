package com.nethackff;

import android.inputmethodservice.Keyboard;
import android.inputmethodservice.KeyboardView;
import android.inputmethodservice.KeyboardView.OnKeyboardActionListener;

public class NetHackKeyboard implements OnKeyboardActionListener
{
	KeyboardView virtualKeyboardView;
	Keyboard virtualKeyboardQwerty;
	Keyboard virtualKeyboardSymbols;
	Keyboard virtualKeyboardSymbolsShift;

	NetHackApp netHackApp;
	
	NetHackKeyboard(NetHackApp app)
	{
		netHackApp = app;
		virtualKeyboardQwerty = new Keyboard(app, R.xml.keyboard_qwerty);
		virtualKeyboardSymbols = new Keyboard(app, R.xml.keyboard_sym);
		virtualKeyboardSymbolsShift = new Keyboard(app, R.xml.keyboard_symshift);
		virtualKeyboardView = (KeyboardView)app.getLayoutInflater().inflate(
				R.layout.input, null);
		virtualKeyboardView.setKeyboard(virtualKeyboardQwerty);
		virtualKeyboardView.setOnKeyboardActionListener(this);
	}
/*
^Z	suspend
^A	again
^R	refresh
^P	last message
^D	kick
^T	'port
^X	show attributes
?	help
!	shell
/	whatis
\	known
#	extended
>
<
^	trap
)[="(	current items
*	combination of items
$	gold
+	spells
_	travel
.	rest
,	pickup
@	toggle pickup
:	look
;	look far

&	?

unused: %'
???		~{}

needed from 2nd page: <>^_[

*/
	
/*
 *     private void handleShift() {
        if (mInputView == null) {
            return;
        }
        
        Keyboard currentKeyboard = mInputView.getKeyboard();
        if (mQwertyKeyboard == currentKeyboard) {
            // Alphabet keyboard
            checkToggleCapsLock();
            mInputView.setShifted(mCapsLock || !mInputView.isShifted());
        } else if (currentKeyboard == mSymbolsKeyboard) {
            mSymbolsKeyboard.setShifted(true);
            mInputView.setKeyboard(mSymbolsShiftedKeyboard);
            mSymbolsShiftedKeyboard.setShifted(true);
        } else if (currentKeyboard == mSymbolsShiftedKeyboard) {
            mSymbolsShiftedKeyboard.setShifted(false);
            mInputView.setKeyboard(mSymbolsKeyboard);
            mSymbolsKeyboard.setShifted(false);
        }
    }

*/
	private void handleShift()
	{
		Keyboard currentKeyboard = virtualKeyboardView.getKeyboard();
//		checkToggleCapsLock();
//		virtualKeyboardView.setShifted(mCapsLock || !mInputView.isShifted());

		if(currentKeyboard == virtualKeyboardQwerty)
		{
			// checkToggleCapsLock();
			virtualKeyboardView.setShifted(/*capslock*/ !virtualKeyboardView.isShifted());
		}
/*
		else if(currentKeyboard == virtualKeyboardSymbols)
		{
			virtualKeyboardSymbols.setShifted(true);
			virtualKeyboardView.setKeyboard(virtualKeyboardSymbolsShift);
			virtualKeyboardSymbolsShift.setShifted(true);
		}
		else if(currentKeyboard == virtualKeyboardSymbolsShift)
		{
            virtualKeyboardSymbolsShift.setShifted(false);
            virtualKeyboardView.setKeyboard(virtualKeyboardSymbols);
            virtualKeyboardSymbols.setShifted(false);
		}
*/
	}
	
	public void onKey(int primaryCode, int[] keyCodes)
	{
		char c = 0;
		if(primaryCode == Keyboard.KEYCODE_DELETE)
		{
			c = 8;
		}
		else if(primaryCode == Keyboard.KEYCODE_SHIFT)
		{
			handleShift();
		}
		else if(primaryCode == Keyboard.KEYCODE_ALT)
		{
			Keyboard current = virtualKeyboardView.getKeyboard();
			if(current == virtualKeyboardSymbolsShift)
			{
				virtualKeyboardView.setKeyboard(virtualKeyboardQwerty);
			}
			else
			{
				//virtualKeyboardSymbols.setShifted(true);
				virtualKeyboardView.setKeyboard(virtualKeyboardSymbolsShift);
				//virtualKeyboardSymbolsShift.setShifted(true);
			}
		}
		else if(primaryCode == Keyboard.KEYCODE_MODE_CHANGE)
		{
			Keyboard current = virtualKeyboardView.getKeyboard();
			if(current == virtualKeyboardSymbols)
			{
				current = virtualKeyboardQwerty;
			}
			else
			{
				current = virtualKeyboardSymbols;
			}
			virtualKeyboardView.setKeyboard(current);
			if(current == virtualKeyboardSymbols)
			{
				current.setShifted(false);
			}
		}
/*
 *        if (attr != null 
                && mInputView != null && mQwertyKeyboard == mInputView.getKeyboard()) {
            int caps = 0;
            EditorInfo ei = getCurrentInputEditorInfo();
            if (ei != null && ei.inputType != EditorInfo.TYPE_NULL) {
                caps = getCurrentInputConnection().getCursorCapsMode(attr.inputType);
            }
            mInputView.setShifted(mCapsLock || caps != 0);
 
*/
		/*
        } else if (primaryCode == Keyboard.KEYCODE_MODE_CHANGE
                && mInputView != null) {
            Keyboard current = mInputView.getKeyboard();
            if (current == mSymbolsKeyboard || current == mSymbolsShiftedKeyboard) {
                current = mQwertyKeyboard;
            } else {
                current = mSymbolsKeyboard;
            }
            mInputView.setKeyboard(current);
            if (current == mSymbolsKeyboard) {
                current.setShifted(false);
            }
        } else {
*/
       	else
		{
			c = (char)primaryCode;			
			if(virtualKeyboardView.getKeyboard() == virtualKeyboardQwerty && virtualKeyboardView.isShifted())
			{
				c = Character.toUpperCase(c);
				virtualKeyboardView.setShifted(false);
			}
		}
		if(c != 0)
		{
			String s = "";
			s += c;
			netHackApp.game.jni.NetHackTerminalSend(s);
		}
	}
	
	public void onPress(int primaryCode)
	{
	}
	public void onRelease(int primaryCode)
	{
	}
	public void onText(CharSequence text)
	{
	}
	public void swipeDown()
	{
	}
	public void swipeLeft()
	{
	}
	public void swipeRight()
	{
	}
	public void swipeUp()
	{
	}
}
