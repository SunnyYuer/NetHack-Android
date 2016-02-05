package com.nethackff;

public class NetHackTerminalState
{
	public NetHackTerminalState()
	{
		numRows = 0;
		numColumns = 0;
		currentRow = 0;
		currentColumn = 0;
	}

	public NetHackTerminalState(int columns, int rows)
	{
		init(columns, rows);
	}

	public void init(int columns, int rows)
	{
		numRows = rows;
		numColumns = columns;

		textBuffer = new char[rows*columns];
		fmtBuffer = new char[rows*columns];

		clearScreen();

		clearChange();
		currentRow = 0;
		currentColumn = 0;
	}
	
	public char getTextAt(int column, int row)
	{
		column -= offsetX;
		row -= offsetY;
		if(column >= 0 && column < numColumns && row >= 0 && row < numRows)
		{
			return textBuffer[row*numColumns + column];
		}
		else
		{
			return ' ';			
		}
	}

	public void setTextAt1(char c, int column, int row)
	{
		column -= offsetX;
		row -= offsetY;
		if(column >= 0 && column < numColumns && row >= 0 && row < numRows)
		{
			textBuffer[row*numColumns + column] = c;
		}
	}

	public int offsetX = 0;
	public int offsetY = 0;
	
	// TODO: Add protection and stuff...
	public char[] textBuffer;
	public char[] fmtBuffer;

	int numRows;
	int numColumns;

	int currentRow;
	int currentColumn;

	int changeColumn1, changeColumn2;
	int changeRow1, changeRow2;

	protected void clearChange()
	{
		changeColumn1 = numColumns;
		changeColumn2 = -1;
		changeRow1 = numRows;
		changeRow2 = -1;
	}

	protected void registerChange(int column, int row)
	{
		if(column < changeColumn1)
		{
			changeColumn1 = column;
		}
		if(column > changeColumn2)
		{
			changeColumn2 = column;
		}
		if(row < changeRow1)
		{
			changeRow1 = row;
		}
		if(row > changeRow2)
		{
			changeRow2 = row;
		}
	}

	public static final int kColBlack = 0;
	public static final int kColRed = 1;
	public static final int kColGreen = 2;
	public static final int kColYellow = 3;
	public static final int kColBlue = 4;
	public static final int kColMagenta = 5;
	public static final int kColCyan = 6;
	public static final int kColWhite = 7;

	int colorForeground = kColWhite, colorBackground = kColBlack;

	char encodeFormat(int foreground, int background, boolean reverse,
			boolean bright, boolean underline)
	{
		if(reverse)
		{
			foreground = 7 - foreground;
			background = 7 - background;
		}
		if(bright)
		{
			foreground += 8;
		}
		if(underline)
		{
			foreground += 16;
		}
		return (char) ((foreground << 3) + background);
	}

	int decodeFormatForeground(char fmt)
	{
		return (fmt >> 3) & 31;
	}

	int decodeFormatBackground(char fmt)
	{
		return fmt & 7;
	}

	char encodeCurrentFormat()
	{
		return encodeFormat(colorForeground, colorBackground, grReverseVideo,
				grBright, grUnderline);
	}

	void clearScreen()
	{
		for(int i = 0; i < numRows*numColumns; i++)
		{
			textBuffer[i] = ' ';
			fmtBuffer[i] = encodeCurrentFormat();
		}
		
		changeColumn1 = 0;
		changeColumn2 = numColumns - 1;
		changeRow1 = 0;
		changeRow2 = numRows - 1;
	}

	void clampCursorPos()
	{
		if(currentRow < 0)
		{
			currentRow = 0;
		}
		else if(currentRow >= numRows)
		{
			// Should we scroll down in this case?
			currentRow = numRows - 1;
		}
		if(currentColumn < 0)
		{
			currentColumn = 0;
		}
		else if(currentColumn >= numColumns)
		{
			currentColumn = numColumns - 1;
		}
	}

	void moveCursorRel(int coldelta, int rowdelta)
	{
		currentRow += rowdelta;
		currentColumn += coldelta;
		clampCursorPos();
	}

	void moveCursorAbs(int newcol, int newrow)
	{
		currentRow = newrow;
		currentColumn = newcol;
		clampCursorPos();
	}

	public void lineFeed()
	{
		currentRow++;
		currentColumn = 0;

		if(currentRow >= numRows)
		{
			for(int row = 1; row < numRows; row++)
			{
				for(int col = 0; col < numColumns; col++)
				{
					setTextAt1(getTextAt(col, row), col, row - 1);
					fmtBuffer[(row - 1) * numColumns + col] = fmtBuffer[row*numColumns + col];
				}
			}
			for(int col = 0; col < numColumns; col++)
			{
				setTextAt1(' ', col, numRows - 1);
				fmtBuffer[(numRows - 1) * numColumns + col] = encodeCurrentFormat();
			}
			currentRow--;

			changeColumn1 = 0;
			changeColumn2 = numColumns - 1;
			changeRow1 = 0;
			changeRow2 = numRows - 1;
		}
	}

	public void writeRaw(char c)
	{
		if(currentColumn >= numColumns)
		{
			lineFeed();
		}

		if (currentColumn < numColumns && currentRow < numRows)
		{
			setTextAt1(c, currentColumn, currentRow);
			fmtBuffer[currentRow * numColumns + currentColumn] = encodeCurrentFormat();

			registerChange(currentColumn, currentRow);
		}
		currentColumn++;
	}

	public void setCharAtPos(char c, int col, int row)
	{
		if (col >= 0 && col < numColumns && row >= 0 && row < numRows)
		{
			setTextAt1(c, col, row);
			fmtBuffer[row * numColumns + col] = encodeCurrentFormat();
			registerChange(col, row);
		}
	}

	public void writeRawStr(String s)
	{
		int len = s.length();
		for (int i = 0; i < len; i++)
		{
			writeRaw(s.charAt(i));
		}
	}

	private static final int ESC_NONE = 0;
	private static final int ESC = 1;
	private static final int ESC_LEFT_SQUARE_BRACKET = 2;

	private int escapeState;

	public void startEscapeSequence(int state)
	{
		escSeqLen = 0;
		escapeState = state;
	}

	public void updateEscapeSequence(char c)
	{
		if(escSeqLen < kMaxEscSeqLen)
		{
			escSeqStored[escSeqLen++] = c;
		}
		switch (escapeState)
		{
			case ESC:
				updateEscapeSequenceEsc(c);
				break;

			case ESC_LEFT_SQUARE_BRACKET:
				updateEscapeSequenceLeftSquareBracket(c);
				break;

			default:
				reportUnknownSequence();
				escapeState = ESC_NONE;
				break;
		}
	}

	public void updateEscapeSequenceEsc(char c)
	{
		switch(c)
		{
			case '[':
				escapeState = ESC_LEFT_SQUARE_BRACKET;
				escSeqArgVal[0] = 0;
				escSeqArgCnt = -1;
				break;

			default:
				reportUnknownSequence();
				escapeState = ESC_NONE;
				break;
		}
	}

	public static final int kMaxEscParam = 16;
	public int[] escSeqArgVal = new int[kMaxEscParam];
	public int escSeqArgCnt = 0;

	public static final int kMaxEscSeqLen = 64; // Not sure...
	public char[] escSeqStored = new char[kMaxEscSeqLen];
	public int escSeqLen = 0;

	public int getEscSeqArgVal(int deflt)
	{
		if (escSeqArgCnt < 0)
		{
			// No arguments specified.
			return deflt;
		}
		else
		{
			return escSeqArgVal[escSeqArgCnt];
		}
	}

	public void reportUnknownChar(char c)
	{
		if(currentColumn > 1)
		{
			lineFeed();
		}
		writeRawStr("Unknown character: " + (int) c);
		lineFeed();
	}

	public void reportUnknownSequence()
	{
		if(currentColumn > 1)
		{
			lineFeed();
		}
		writeRawStr("Unknown Esc sequence: ");
		for(int i = 0; i < escSeqLen; i++)
		{
			writeRaw(escSeqStored[i]);
		}
		lineFeed();
	}

	public boolean grReverseVideo = false;
	public boolean grBright = false;
	public boolean grUnderline = false;

	public void selectGraphicRendition(int arg)
	{
		if(arg >= 30 && arg <= 37)
		{
			colorForeground = arg - 30;
			return;
		}
		if(arg >= 40 && arg <= 47)
		{
			colorBackground = arg - 40;
			return;
		}
		switch(arg)
		{
			case 0:
				grReverseVideo = false;
				colorForeground = kColWhite;
				colorBackground = kColBlack;
				grBright = false; // Not sure
				grUnderline = false;
				break;
			case 1:
				grBright = true;
				break;
			case 2:
				grBright = false;
				break;
			case 3:
				reportUnknownSequence();
				break;
			case 4:
				grUnderline = true;
				break;
			case 5:
			case 6:
				reportUnknownSequence();
				break;
			case 7:
				grReverseVideo = true;
				break;
			default:
				reportUnknownSequence();
				break;
		}
	}

	public void selectGraphicRendition()
	{
		if(escSeqArgCnt < 0)
		{
			selectGraphicRendition(0);
		}
		else
		{
			for(int i = 0; i <= escSeqArgCnt; i++)
			{
				selectGraphicRendition(escSeqArgVal[i]);
			}
		}
	}

	public void updateEscapeSequenceLeftSquareBracket(char c)
	{
		switch(c)
		{
			case 'B': // Move cursor down n lines
				moveCursorRel(0, getEscSeqArgVal(1));
				escapeState = ESC_NONE;
				return;
			case 'C': // Move cursor right n lines
				moveCursorRel(getEscSeqArgVal(1), 0);
				escapeState = ESC_NONE;
				return;
			case 'D': // Move cursor left n lines
				moveCursorRel(-getEscSeqArgVal(1), 0);
				escapeState = ESC_NONE;
				return;
			case 'A': // Move cursor up n lines
				moveCursorRel(0, -getEscSeqArgVal(1));
				escapeState = ESC_NONE;
				return;
			case 'H': // Cursor home
				if(escSeqArgCnt == 1)
				{
					moveCursorAbs(escSeqArgVal[1] - 1, escSeqArgVal[0] - 1);
				}
				else
				{
					moveCursorAbs(0, 0);
				}
				escapeState = ESC_NONE;
				return;
			case 'J': // Clear screen
				// TODO: Read arguments here.
				clearScreen();
				escapeState = ESC_NONE;
				return;
			case 'K':
				if(getEscSeqArgVal(0) == 0)
				{
					// Clear line from cursor right
					for(int i = currentColumn; i < numColumns; i++)
					{
						setCharAtPos(' ', i, currentRow);
					}
				}
				else if(getEscSeqArgVal(0) == 1)
				{
					// Clear line from cursor left
					for(int i = currentColumn; i >= 0; i--)
					{
						setCharAtPos(' ', i, currentRow);
					}
				}
				else if (getEscSeqArgVal(0) == 2)
				{
					for(int i = 0; i < numColumns; i++)
					{
						setCharAtPos(' ', i, currentRow);
					}
				}
				else
				{
					reportUnknownSequence();
				}
				escapeState = ESC_NONE;
				return;
			case 'm': // Select graphic rendition
				selectGraphicRendition();
				escapeState = ESC_NONE;
				return;
		}
		if(c >= '0' && c <= '9')
		{
			if(escSeqArgCnt == -1)
			{
				escSeqArgCnt = 0;
			}
			escSeqArgVal[escSeqArgCnt] = escSeqArgVal[escSeqArgCnt]*10 + (c - '0');
		}
		else if(c == ';')
		{
			escSeqArgCnt++;
			escSeqArgVal[escSeqArgCnt] = 0;
		}
		else
		{
			reportUnknownSequence();
			escapeState = ESC_NONE;
		}
	}

	public void write(char c)
	{
		switch(c)
		{
			case 0: // NUL
				break;
			case 7: // BEL
				break;
			case 8: // BS
				if (currentColumn > 0)
					currentColumn--;
				break;
			case 9: // HT
				// TODO
				reportUnknownChar(c);
				break;
			case 13:
				currentColumn = 0;
				return;
			case 10: // CR
			case 11: // VT
			case 12: // LF
				lineFeed();
				return;
			case 14: // SO
				// TODO
				break;
			case 15: // SI
				// TODO
				break;
			case 24: // CAN
			case 26: // SUB
				// TODO
				// break;
			case 0x9b: // CSI
				reportUnknownChar(c);
				break;
			case 27: // ESC
				startEscapeSequence(ESC);
				return;
		};

		if(escapeState == ESC_NONE)
		{
			if(c >= 32)
			{
				writeRaw(c);
			}
		}
		else
		{
			updateEscapeSequence(c);
		}
	}

	public void write(String s)
	{
		int len = s.length();
		for (int i = 0; i < len; i++)
		{
			write(s.charAt(i));
		}
	}

	public String getContents()
	{
		String r = "";
		for (int i = 0; i < numRows; i++)
		{
			r += getRow(i);
			r += '\n';
		}
		return r;
	}

	public String getRow(int row)
	{
		String r;
		r = "";
		row -= offsetY;
		if(row >= 0 && row < numRows)
		{
			int offs = row*numColumns;
			for (int i = 0; i < numColumns; i++)
			{
				// TODO: Maybe do something with offsetX here?
				r += textBuffer[offs + i];
			}
		}
		return r;
	}
};
