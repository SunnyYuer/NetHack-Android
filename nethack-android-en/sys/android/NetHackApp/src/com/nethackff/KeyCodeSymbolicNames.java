package com.nethackff;

import android.util.SparseArray;
import android.view.KeyEvent;
/**
 * 
 * @author michaelbarlow7
 * This is basically a slightly modified copy of the String mappings for KeyCodes in the KeyEvent class
 * from API level 12 and up. The "keyCodeToString" method was introduced in this API.
 * This is used to get names for keybindings. A little hacky I guess.
 */
public class KeyCodeSymbolicNames
{
	public static String keyCodeToString(int keyCode)
	{
		//TODO: Allow for API >= 12 to use normal KeyEvent api
		String name = KEYCODE_SYMBOLIC_NAMES.get(keyCode);
		if (name != null)
		{
			return name;
		}
		return String.valueOf(keyCode);
	}
    private static final SparseArray<String> KEYCODE_SYMBOLIC_NAMES = new SparseArray<String>();

	static{
        SparseArray<String> names = KEYCODE_SYMBOLIC_NAMES;
        names.append(KeyEvent.KEYCODE_UNKNOWN, "UNKNOWN");
        names.append(KeyEvent.KEYCODE_SOFT_LEFT, "SOFT_LEFT");
        names.append(KeyEvent.KEYCODE_SOFT_RIGHT, "SOFT_RIGHT");
        names.append(KeyEvent.KEYCODE_HOME, "HOME");
        names.append(KeyEvent.KEYCODE_BACK, "BACK");
        names.append(KeyEvent.KEYCODE_CALL, "CALL");
        names.append(KeyEvent.KEYCODE_ENDCALL, "ENDCALL");
        names.append(KeyEvent.KEYCODE_0, "0");
        names.append(KeyEvent.KEYCODE_1, "1");
        names.append(KeyEvent.KEYCODE_2, "2");
        names.append(KeyEvent.KEYCODE_3, "3");
        names.append(KeyEvent.KEYCODE_4, "4");
        names.append(KeyEvent.KEYCODE_5, "5");
        names.append(KeyEvent.KEYCODE_6, "6");
        names.append(KeyEvent.KEYCODE_7, "7");
        names.append(KeyEvent.KEYCODE_8, "8");
        names.append(KeyEvent.KEYCODE_9, "9");
        names.append(KeyEvent.KEYCODE_STAR, "STAR");
        names.append(KeyEvent.KEYCODE_POUND, "POUND");
        names.append(KeyEvent.KEYCODE_DPAD_UP, "DPAD_UP");
        names.append(KeyEvent.KEYCODE_DPAD_DOWN, "DPAD_DOWN");
        names.append(KeyEvent.KEYCODE_DPAD_LEFT, "DPAD_LEFT");
        names.append(KeyEvent.KEYCODE_DPAD_RIGHT, "DPAD_RIGHT");
        names.append(KeyEvent.KEYCODE_DPAD_CENTER, "DPAD_CENTER");
        names.append(KeyEvent.KEYCODE_VOLUME_UP, "VOLUME_UP");
        names.append(KeyEvent.KEYCODE_VOLUME_DOWN, "VOLUME_DOWN");
        names.append(KeyEvent.KEYCODE_POWER, "POWER");
        names.append(KeyEvent.KEYCODE_CAMERA, "CAMERA");
        names.append(KeyEvent.KEYCODE_CLEAR, "CLEAR");
        names.append(KeyEvent.KEYCODE_A, "A");
        names.append(KeyEvent.KEYCODE_B, "B");
        names.append(KeyEvent.KEYCODE_C, "C");
        names.append(KeyEvent.KEYCODE_D, "D");
        names.append(KeyEvent.KEYCODE_E, "E");
        names.append(KeyEvent.KEYCODE_F, "F");
        names.append(KeyEvent.KEYCODE_G, "G");
        names.append(KeyEvent.KEYCODE_H, "H");
        names.append(KeyEvent.KEYCODE_I, "I");
        names.append(KeyEvent.KEYCODE_J, "J");
        names.append(KeyEvent.KEYCODE_K, "K");
        names.append(KeyEvent.KEYCODE_L, "L");
        names.append(KeyEvent.KEYCODE_M, "M");
        names.append(KeyEvent.KEYCODE_N, "N");
        names.append(KeyEvent.KEYCODE_O, "O");
        names.append(KeyEvent.KEYCODE_P, "P");
        names.append(KeyEvent.KEYCODE_Q, "Q");
        names.append(KeyEvent.KEYCODE_R, "R");
        names.append(KeyEvent.KEYCODE_S, "S");
        names.append(KeyEvent.KEYCODE_T, "T");
        names.append(KeyEvent.KEYCODE_U, "U");
        names.append(KeyEvent.KEYCODE_V, "V");
        names.append(KeyEvent.KEYCODE_W, "W");
        names.append(KeyEvent.KEYCODE_X, "X");
        names.append(KeyEvent.KEYCODE_Y, "Y");
        names.append(KeyEvent.KEYCODE_Z, "Z");
        names.append(KeyEvent.KEYCODE_COMMA, "COMMA");
        names.append(KeyEvent.KEYCODE_PERIOD, "PERIOD");
        names.append(KeyEvent.KEYCODE_ALT_LEFT, "ALT_LEFT");
        names.append(KeyEvent.KEYCODE_ALT_RIGHT, "ALT_RIGHT");
        names.append(KeyEvent.KEYCODE_SHIFT_LEFT, "SHIFT_LEFT");
        names.append(KeyEvent.KEYCODE_SHIFT_RIGHT, "SHIFT_RIGHT");
        names.append(KeyEvent.KEYCODE_TAB, "TAB");
        names.append(KeyEvent.KEYCODE_SPACE, "SPACE");
        names.append(KeyEvent.KEYCODE_SYM, "SYM");
        names.append(KeyEvent.KEYCODE_EXPLORER, "EXPLORER");
        names.append(KeyEvent.KEYCODE_ENVELOPE, "ENVELOPE");
        names.append(KeyEvent.KEYCODE_ENTER, "ENTER");
        names.append(KeyEvent.KEYCODE_DEL, "DEL");
        names.append(KeyEvent.KEYCODE_GRAVE, "GRAVE");
        names.append(KeyEvent.KEYCODE_MINUS, "MINUS");
        names.append(KeyEvent.KEYCODE_EQUALS, "EQUALS");
        names.append(KeyEvent.KEYCODE_LEFT_BRACKET, "LEFT_BRACKET");
        names.append(KeyEvent.KEYCODE_RIGHT_BRACKET, "RIGHT_BRACKET");
        names.append(KeyEvent.KEYCODE_BACKSLASH, "BACKSLASH");
        names.append(KeyEvent.KEYCODE_SEMICOLON, "SEMICOLON");
        names.append(KeyEvent.KEYCODE_APOSTROPHE, "APOSTROPHE");
        names.append(KeyEvent.KEYCODE_SLASH, "SLASH");
        names.append(KeyEvent.KEYCODE_AT, "AT");
        names.append(KeyEvent.KEYCODE_NUM, "NUM");
        names.append(KeyEvent.KEYCODE_HEADSETHOOK, "HEADSETHOOK");
        names.append(KeyEvent.KEYCODE_FOCUS, "FOCUS");
        names.append(KeyEvent.KEYCODE_PLUS, "PLUS");
        names.append(KeyEvent.KEYCODE_MENU, "MENU");
        names.append(KeyEvent.KEYCODE_NOTIFICATION, "NOTIFICATION");
        names.append(KeyEvent.KEYCODE_SEARCH, "SEARCH");
        names.append(KeyEvent.KEYCODE_MEDIA_PLAY_PAUSE, "MEDIA_PLAY_PAUSE");
        names.append(KeyEvent.KEYCODE_MEDIA_STOP, "MEDIA_STOP");
        names.append(KeyEvent.KEYCODE_MEDIA_NEXT, "MEDIA_NEXT");
        names.append(KeyEvent.KEYCODE_MEDIA_PREVIOUS, "MEDIA_PREVIOUS");
        names.append(KeyEvent.KEYCODE_MEDIA_REWIND, "MEDIA_REWIND");
        names.append(KeyEvent.KEYCODE_MEDIA_FAST_FORWARD, "MEDIA_FAST_FORWARD");
        names.append(KeyEvent.KEYCODE_MUTE, "MUTE");
    };
}
