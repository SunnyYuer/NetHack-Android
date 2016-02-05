package com.nethackff;

import android.content.Context;
import android.graphics.Bitmap;
import android.graphics.Canvas;
import android.graphics.Color;
import android.graphics.Paint;
import android.graphics.PorterDuff.Mode;
import android.graphics.PorterDuffColorFilter;
import android.graphics.Rect;
import android.graphics.RectF;
import android.graphics.Typeface;
import android.util.Log;
import android.view.View;

public class NetHackTerminalView extends NetHackView
{
	private boolean drawCursor = true;
	private boolean whiteBackgroundMode = false;

	public boolean reformatText = false;	

	public Bitmap fontBitmap;
	
	public final int ybackgroffs = 2;
	public int displayedLinesOnReformat = 0;

	public int zoomDelta = 0;
	Paint bitmapPaint;
	Paint textPaint;
	
	private int textSize = 10;

	public void zoomIn()
	{
		if(zoomDelta < 10)
		{
			zoomDelta += 2;
			zoomChanged();
		}
	}
	
	public void zoomOut()
	{
		if(zoomDelta >= 4 - textSize)	// TEMP
		{
			zoomDelta -= 2;
			zoomChanged();
		}
	}

	public void updateZoom()
	{
		updateTextSize();
	}

	public int getNumDisplayedLines()	// Override
	{
		if(reformatText)
		{
			return displayedLinesOnReformat > sizeY ? displayedLinesOnReformat : sizeY;
		}
		else
		{
			return sizeY;			
		}
	}
	
	public void setTextSize(int sz)
	{
		textSize = sz;
		updateTextSize();
	}

	public void updateTextSize()
	{
		textPaint.setTextSize(Math.max(textSize + zoomDelta, 4));
		squareSizeY = (int)Math.ceil(textPaint.getFontSpacing());
		squareSizeX = (int)textPaint.measureText("X", 0, 1);

		computeSizePixels();
	}

	public void setDrawCursor(boolean b)
	{
		if(b != drawCursor)
		{
			terminal.registerChange(terminal.currentColumn, terminal.currentRow);
			drawCursor = b;
		}
	}
	
	public boolean getDrawCursor()
	{
		return drawCursor;
	}

	public void write(String s)
	{
		terminal.write(s);

		if(terminal.changeColumn1 <= terminal.changeColumn2)
		{
			if(drawCursor)
			{
				// Since we will draw the cursor at the current position, we should probably consider
				// the current position as a change.
				terminal.registerChange(terminal.currentColumn, terminal.currentRow);
			}

			Rect cliprect = new Rect();
			cliprect.bottom = computeCoordY(terminal.changeRow2) + squareSizeY;
			cliprect.top = computeCoordY(terminal.changeRow1);
			cliprect.right = computeCoordX(terminal.changeColumn2) + squareSizeX;
			cliprect.left = computeCoordX(terminal.changeColumn1);
			invalidate(cliprect);
		}
	}
	
	public NetHackTerminalView(Context context, NetHackTerminalState term)
	{
		super(context);

		terminal = term;

		// Paint paint = createPaint();
		textPaint = new Paint();
		textPaint.setTypeface(Typeface.MONOSPACE);
		textPaint.setAntiAlias(true);

		bitmapPaint = new Paint();
		bitmapPaint.setAntiAlias(true);

		offsetX = 0;
		offsetY = 0;
		sizeX = term.numColumns;
		sizeY = term.numRows;

		updateTextSize();

		updateBackground();
	}
	public void setWhiteBackgroundMode(boolean b)
	{
		whiteBackgroundMode = b;
		updateBackground();
	}
	public void updateBackground()
	{
		if(whiteBackgroundMode)
		{
			setBackgroundColor(0xffffffff);
		}
		else
		{
			setBackgroundColor(0xff000000);
		}
	}

	enum ColorSet
	{
		AnsiTerminal,
		IBM,
		Amiga
	};
	ColorSet colorSet = ColorSet.AnsiTerminal;

	void setPaintColorForegroundAmiga(Paint paint, int col, Paint bitmappaint, boolean cursor)
	{
		paint.setFakeBoldText(false);
		if((col & 16) != 0)
		{
			paint.setUnderlineText(true);
			col &= ~16;
		}
		else
		{
			paint.setUnderlineText(false);
		}

		int pen;
		if(whiteBackgroundMode)
		{
			pen = colSetAmiga_fgpens_whitebg[col];
		}
		else
		{
			pen = colSetAmiga_fgpens[col];
		}
		
		if(cursor)
		{
			pen = (0xf & ~pen);
		}
		int argb = Color.argb(0xff, ((colSetAmiga_rgb[pen] & 0xf00) >> 8)*0x11, ((colSetAmiga_rgb[pen] & 0x0f0) >> 4)*0x11, (colSetAmiga_rgb[pen] & 0x00f)*0x11);
		paint.setColor(argb);
		bitmappaint.setColorFilter(new PorterDuffColorFilter(argb, Mode.SRC_ATOP));
		
	}

	final int colSetAmiga_rgb[] = {	0x000, 0xfff, 0x830, 0x7ac, 0x181, 0xc06, 0x23e, 0xc00,
									0x888, 0xf60, 0x4f4, 0xff0, 0x4af, 0xf8f, 0x8ff, 0xf00 };
	final int colSetAmiga_fgpens[] =			{ 0, 7, 4, 2, 6, 5, 3, 8, 1, 9, 10, 11, 12, 13, 14, 1 };
	final int colSetAmiga_fgpens_whitebg[] =	{ 1, 7, 4, 2, 6, 5, 3, 8, 0, 9, 10, 11, 12, 13, 14, 0 };
	final int colSetAmiga_bgpens[] =			{ 0, 7, 4, 2, 6, 5, 3, 8, 1, 9, 10, 11, 12, 13, 14, 1 };

	
	final int colSetIBM_rgb[] =	{	0x000, 0x00a, 0x0a0, 0x0aa, 0xa00, 0xa0a, 0xa50, 0xaaa,
									0x555, 0x55f, 0x5f5, 0x5ff, 0xf55, 0xf5f, 0xff5, 0xfff };
	final int colSetIBM_fgpens[] =				{ 0, 4, 2, 6, 1, 5, 3, 7, 8, 12, 10, 14, 9, 13, 11, 15 };
	final int colSetIBM_fgpens_whitebg[] =		{ 15, 4, 2, 6, 1, 5, 3, 7, 8, 12, 10, 14, 9, 13, 11, 0 };

	
	void setPaintColorForegroundIBM(Paint paint, int col, Paint bitmappaint, boolean cursor)
	{
		paint.setFakeBoldText(false);
		if((col & 16) != 0)
		{
			paint.setUnderlineText(true);
			col &= ~16;
		}
		else
		{
			paint.setUnderlineText(false);
		}

		int pen;
		if(whiteBackgroundMode)
		{
			pen = colSetIBM_fgpens_whitebg[col];
		}
		else
		{
			pen = colSetIBM_fgpens[col];
		}

		if(cursor)
		{
			pen = (0xf & ~pen);
		}
		int argb = Color.argb(0xff, ((colSetIBM_rgb[pen] & 0xf00) >> 8)*0x11, ((colSetIBM_rgb[pen] & 0x0f0) >> 4)*0x11, (colSetIBM_rgb[pen] & 0x00f)*0x11);
		paint.setColor(argb);
		bitmappaint.setColorFilter(new PorterDuffColorFilter(argb, Mode.SRC_ATOP));
		
	}

	
	void setPaintColorForeground(Paint paint, int col, Paint bitmappaint, boolean cursor)
	{
		if(colorSet == ColorSet.Amiga)
		{
			setPaintColorForegroundAmiga(paint, col, bitmappaint, cursor);
			return;
		}

		if(colorSet == ColorSet.IBM)
		{
			setPaintColorForegroundIBM(paint, col, bitmappaint, cursor);
			return;
		}
		
		if(cursor)
		{
			boolean bold = false;
			if(col >= 8)
			{
				col -= 8;
				bold = true;
			}
			col = 7 - col;
			if(bold)
			{
				col += 8;
			}
		}

		if((col & 8) != 0)
		{
			paint.setFakeBoldText(true);
			col &= ~8;
		}
		else
		{
			paint.setFakeBoldText(false);
		}
		if((col & 16) != 0)
		{
			paint.setUnderlineText(true);
			col &= ~16;
		}
		else
		{
			paint.setUnderlineText(false);
		}

		int argb;		
		if(whiteBackgroundMode)
		{
			switch(col)
			{
				case NetHackTerminalState.kColBlack:
					argb = Color.argb(0xff, 0xff, 0xff, 0xff);
					break;
				case NetHackTerminalState.kColRed:
					argb = Color.argb(0xff, 0xff, 0x00, 0x00);
					break;
				case NetHackTerminalState.kColGreen:
					argb = Color.argb(0xff, 0x00, 0xc0, 0x00);
					break;
				case NetHackTerminalState.kColYellow:
					argb = Color.argb(0xff, 0xb0, 0xb0, 0x00);
					break;
				case NetHackTerminalState.kColBlue:
					argb = Color.argb(0xff, 0x00, 0x00, 0xff);
					break;
				case NetHackTerminalState.kColMagenta:
					argb = Color.argb(0xff, 0xff, 0x00, 0xff);
					break;
				case NetHackTerminalState.kColCyan:
					argb = Color.argb(0xff, 0x00, 0xb0, 0xb0);
					break;
				case NetHackTerminalState.kColWhite:
					argb = Color.argb(0xff, 0x00, 0x00, 0x00);
					break;
				default:
					argb = Color.argb(0x80, 0x80, 0x80, 0x80);
					break;
			}
		}
		else
		{
			switch(col)
			{
				case NetHackTerminalState.kColBlack:
					argb = Color.argb(0xff, 0x00, 0x00, 0x00);
					break;
				case NetHackTerminalState.kColRed:
					argb = Color.argb(0xff, 0xff, 0x00, 0x00);
					break;
				case NetHackTerminalState.kColGreen:
					argb = Color.argb(0xff, 0x00, 0xff, 0x00);
					break;
				case NetHackTerminalState.kColYellow:
					argb = Color.argb(0xff, 0xff, 0xff, 0x00);
					break;
				case NetHackTerminalState.kColBlue:
					argb = Color.argb(0xff, 0x00, 0x00, 0xff);
					break;
				case NetHackTerminalState.kColMagenta:
					argb = Color.argb(0xff, 0xff, 0x00, 0xff);
					break;
				case NetHackTerminalState.kColCyan:
					argb = Color.argb(0xff, 0x00, 0xff, 0xff);
					break;
				case NetHackTerminalState.kColWhite:
					argb = Color.argb(0xff, 0xff, 0xff, 0xff);
					break;
				default:
					argb = Color.argb(0x80, 0x80, 0x80, 0x80);
					break;
			}
		}

		paint.setColor(argb);
		bitmappaint.setColorFilter(new PorterDuffColorFilter(argb, Mode.SRC_ATOP));
	}

	void setPaintColorBackgroundAmiga(Paint paint, int col, boolean cursor)
	{
		int pen = colSetAmiga_bgpens[col];

		if(cursor)
		{
			pen = (0xf & ~pen);
		}
		int argb = Color.argb(0xff, ((colSetAmiga_rgb[pen] & 0xf00) >> 8)*0x11, ((colSetAmiga_rgb[pen] & 0x0f0) >> 4)*0x11, (colSetAmiga_rgb[pen] & 0x00f)*0x11);
		paint.setColor(argb);
		//paint.setColorFilter(new PorterDuffColorFilter(argb, Mode.MULTIPLY));
	}
	
	void setPaintColorBackground(Paint paint, int col, boolean cursor)
	{
		if(colorSet == ColorSet.Amiga && !whiteBackgroundMode)
		{
			setPaintColorBackgroundAmiga(paint, col, cursor);
			return;
		}

		if(cursor)
		{
			col = 7 - col;
		}
		switch(col)
		{
			case NetHackTerminalState.kColBlack:
				if(whiteBackgroundMode)
				{
					paint.setARGB(0xff, 0xff, 0xff, 0xff);
				}
				else
				{
					paint.setARGB(0xff, 0x00, 0x00, 0x00);
				}
				break;
			case NetHackTerminalState.kColRed:
				paint.setARGB(0xff, 0xff, 0x00, 0x00);
				break;
			case NetHackTerminalState.kColGreen:
				paint.setARGB(0xff, 0x00, 0xff, 0x00);
				break;
			case NetHackTerminalState.kColYellow:
				paint.setARGB(0xff, 0xff, 0xff, 0x00);
				break;
			case NetHackTerminalState.kColBlue:
				paint.setARGB(0xff, 0x00, 0x00, 0xff);
				break;
			case NetHackTerminalState.kColMagenta:
				paint.setARGB(0xff, 0xff, 0x00, 0xff);
				break;
			case NetHackTerminalState.kColCyan:
				paint.setARGB(0xff, 0x00, 0xff, 0xff);
				break;
			case NetHackTerminalState.kColWhite:
				if(whiteBackgroundMode)
				{
					paint.setARGB(0xff, 0x00, 0x00, 0x00);
				}
				else
				{
					paint.setARGB(0xff, 0xff, 0xff, 0xff);
				}
				break;
			default:
				paint.setARGB(0x80, 0x80, 0x80, 0x80);
				break;
		}
	}

	protected void onDraw(Canvas canvas)
	{
		pendingRedraw = false;
		if(reformatText)
		{
			// TEMP - should probably check how much is used:
			int numColsNeeded = 80;
			if(sizeX < numColsNeeded)
			{
				onDrawReformat(canvas);
				return;
			}
		}
		onDrawFixed(canvas);

		// TEMP
/*
		Paint p = new Paint();
		int argb = Color.argb(0xff, 0xff, 0xff, 0xff);
		p.setColor(argb);
		canvas.drawLine(desiredCenterPosX - 5, desiredCenterPosY, desiredCenterPosX + 5, desiredCenterPosY, p);
		canvas.drawLine(desiredCenterPosX, desiredCenterPosY - 5, desiredCenterPosX, desiredCenterPosY + 5, p);

		int sx = sizePixelsX;
		int sy = sizePixelsY;
		canvas.drawLine(1, 1, 1, sy - 2, p);
		canvas.drawLine(sx - 2, 1, sx - 2, sy - 2, p);
		canvas.drawLine(1, 1, sx - 2, 1, p);
		canvas.drawLine(1, sy - 2, sx - 2, sy - 2, p);
*/
	}
	
	
	protected void drawRowBackground(Canvas canvas,
			int x, final int y, final char []fmtBuffer,
			final int buffOffs, final int numChars, final int cursorIndex)
	{
		int currentx1 = -1;
		int currentcolor = -1;
		boolean currentcursor = false;

		for(int index = 0; index < numChars; x += squareSizeX, index++)
		{
			char fmt;
			int buffIndex = buffOffs + index;
			if(buffIndex >= 0 && buffIndex < fmtBuffer.length)
			{
				fmt = fmtBuffer[buffIndex];
			}
			else
			{
				fmt = 0;	// Not sure!				
			}
			int color = terminal.decodeFormatBackground(fmt);

			boolean cursor = false;
			if(cursorIndex == index && drawCursor)
			{
				cursor = true;
			}
			if(color == currentcolor && cursor == currentcursor)
			{
				continue;
			}
			if(currentx1 >= 0)
			{
				setPaintColorBackground(textPaint, currentcolor, currentcursor);
				canvas.drawRect(currentx1, y, x, y + squareSizeY, textPaint);
			}
			currentx1 = x;
			currentcolor = color;
			currentcursor = cursor;
		}
		setPaintColorBackground(textPaint, currentcolor, currentcursor);
		canvas.drawRect(currentx1, y, x, y + squareSizeY, textPaint);
	}	

	protected void drawText(Canvas canvas, String currentstr, float currentx1, float y, Paint textPaint)
	{
		// For some reason, despite using a fixed width font, while the value of a call to measureText()
		// on the string gives the expected result, I was seeing some misalignment of the characters
		// for example when using a 15pt font on Nexus, as if some extra horizontal spacing was sneaking
		// in, when using this simple call:
		//		canvas.drawText(currentstr, 0, currentstr.length(),
		//				(float)currentx1, (float)y, textPaint);
		// To avoid the possibility of this, using drawPosText() instead of drawText() seems to work
		// better (though it might add some overhead for building the array).
		
		int len = currentstr.length();
		float positions[] = new float[len*2];

		float x1 = currentx1;
		char []newstr = new char[len];
		int k = 0;
		boolean foundspecial = false;
		for(int i = 0; i < len; i++)
		{
			positions[k++] = x1;
			positions[k++] = y;
			x1 += squareSizeX;
			char c = currentstr.charAt(i);

			if(c >= 0x7000 && fontBitmap != null)
			{
				c = ' ';
				foundspecial = true;
			}

			newstr[i] = c;
		}
		canvas.drawPosText(newstr, 0, len, positions, textPaint);

		if(fontBitmap != null && foundspecial)
		{
			x1 = currentx1;
			for(int i = 0; i < len; i++)
			{
				char c = currentstr.charAt(i);
				if(c >= 0x7000)
				{
					float topy = y - squareSizeY + ybackgroffs;

					int charindex = c - 0x7000 - 32;
					int bitmapsquareSizeX = 8;
					int bitmapsquareSizeY = 8;
					//canvas.drawBitmap(fontBitmap, new Rect(charindex*bitmapsquareSizeX, 0, (charindex + 1)*bitmapsquareSizeX - 1, bitmapsquareSizeY - 1), new RectF(x1, topy, x1 + squareSizeX - 1, topy + squareSizeY - 1), paint); 			
					//canvas.drawBitmap(fontBitmap, new Rect(charindex*bitmapsquareSizeX, 0, (charindex + 1)*bitmapsquareSizeX - 1, bitmapsquareSizeY - 1), new RectF(x1, topy, x1 + squareSizeX, topy + squareSizeY), paint); 			
					canvas.drawBitmap(fontBitmap, new Rect(charindex*bitmapsquareSizeX, 0, (charindex + 1)*bitmapsquareSizeX, bitmapsquareSizeY), new RectF(x1, topy, x1 + squareSizeX, topy + squareSizeY), bitmapPaint);
				}
				x1 += squareSizeX;
			}
		}

	}
	
	protected void drawRowForeground(Canvas canvas,
			int x, final int y,
			final char []txtBuffer, final char []fmtBuffer,
			final int buffOffs, final int numChars, final int cursorIndex)
	{
		int currentx1 = -1;
		int currentcolor = -1;
		boolean currentcursor = false;
		String currentstr = "";
		for(int index = 0; index < numChars; x += squareSizeX, index++)
		{
			char c;
			char fmt;
			int buffIndex = buffOffs + index;
			if(buffIndex >= 0 && buffIndex < fmtBuffer.length)
			{
				fmt = fmtBuffer[buffIndex];
				c = txtBuffer[buffIndex];
			}
			else
			{
				fmt = 0;	// Not sure!				
				c = ' ';
			}
			int color = terminal.decodeFormatForeground(fmt);

			boolean cursor = false;
			if(cursorIndex == index && drawCursor)
			{
				cursor = true;
			}

			if(color == currentcolor && cursor == currentcursor)
			{
				currentstr += c;
				continue;
			}
			if(currentx1 >= 0)
			{
				setPaintColorForeground(textPaint, currentcolor, bitmapPaint, currentcursor);
				drawText(canvas, currentstr, (float)currentx1, (float)y, textPaint);
			}
			currentx1 = x;
			currentcolor = color;
			currentcursor = cursor;
			currentstr = "" + c;
		}
		setPaintColorForeground(textPaint, currentcolor, bitmapPaint, currentcursor);
		drawText(canvas, currentstr,
				(float)currentx1, (float)y, textPaint);
	}


	protected int FindLastNonEmptyPosOnLine(int termYNoOffs, int viewCols)
	{
		int rowTerm = termYNoOffs + offsetY;

		if(rowTerm >= 0 && rowTerm < terminal.numRows)
		{
			char c;//, fmt;
			for(int i = terminal.numColumns - 1; i >= 0; i--)
			{
				//fmt = terminal.fmtBuffer[rowTerm*terminal.numColumns + i];
				c = terminal.textBuffer[rowTerm*terminal.numColumns + i];
				if(c != ' ')
				{
					return i;
				}
				if(drawCursor && rowTerm == terminal.currentRow && i == terminal.currentColumn)
				{
					// This is a visible cursor - shouldn't be treated as an empty position even
					// if the character there is a space.
					return i;
				}
			}
		}
		return -1;
	}
	
	protected void onDrawReformat(Canvas canvas)
	{
		int viewCols = sizeX;

		char []rowTxt = new char[viewCols];
		char []rowFmt = new char[viewCols];

for(int pass = 0; pass < 2; pass++)
{
		for(int i = 0; i < viewCols; i++)
		{
			rowTxt[i] = ' ';
			rowFmt[i] = 0;
		}

		int viewX = 0, viewY = 0;
		displayedLinesOnReformat = 0;
		int displayedLines = 0;

		final boolean stripspaces = false;

//		final int strlen = str.length();
//		int currentwordstart = -1;
//		String currentword = "";
//		String currentline = "";
		int col = 0;
		int termXNoOffs = 0, termYNoOffs = 0;
//		for(int pos = 0; pos < strlen; pos++)
// TEMP
		int lastnonemptyposonline = FindLastNonEmptyPosOnLine(termYNoOffs, viewCols);
		char lastchar = 0;						// Previous character we looked at.
		boolean lastcharwascursor = false;		// True if lastchar was a visible cursor.
		boolean foundNonspaceOnLine = false;

		int linebreakcnt = 0;
		int cursorindex = -1;					// Index of the cursor in rowTxt[], when found.
		while(true)
		{
			char currentchar;	// The character at the position we are reading.
			char currentfmt;	// Format code for currentchar.
			boolean iscursor = false;	// True if currentchar is a visible cursor.
			boolean last = false;
			boolean linebreak = false;
			boolean addchar = false;
			boolean breaklast = true;
			if(linebreakcnt == 0)
			{
				addchar = true;

				int colTerm = termXNoOffs + offsetX;
				int rowTerm = termYNoOffs + offsetY;
				if(rowTerm >= terminal.numRows)
				{
					last = true;
				}
				if(colTerm >= 0 && colTerm < terminal.numColumns
						&& rowTerm >= 0 && rowTerm < terminal.numRows)
				{
					currentfmt = terminal.fmtBuffer[rowTerm*terminal.numColumns + colTerm];
					currentchar = terminal.textBuffer[rowTerm*terminal.numColumns + colTerm];
				}
				else
				{
					currentfmt = 0;	// Not sure!
					currentchar = ' ';
				}
				// Check if the cursor is supposed to be visible at this character's position.
				if(drawCursor && rowTerm == terminal.currentRow && colTerm == terminal.currentColumn)
				{
					iscursor = true;
				}
				if(currentchar != ' ' || iscursor)
				{
					foundNonspaceOnLine = true;
				}
				termXNoOffs++;
				boolean pastlast = termXNoOffs + offsetX > lastnonemptyposonline + 1;
				if(termXNoOffs + offsetX >= terminal.numColumns || pastlast)
				{
					termXNoOffs = 0;
					termYNoOffs++;
					lastnonemptyposonline = FindLastNonEmptyPosOnLine(termYNoOffs, viewCols);
					if(stripspaces)
					{
						if(!foundNonspaceOnLine)
						{
							linebreak = true;
							linebreakcnt = 1;
						}
					}
					else if(pastlast)
					{
						addchar = false;
						breaklast = false;
						linebreak = true;
						linebreakcnt = 0;
					}
					foundNonspaceOnLine = false;
				}
			
				if(stripspaces)
				{
					if(currentchar == ' ' && lastchar == ' ' && !last && !linebreak && !iscursor && !lastcharwascursor)
					{
						continue;
					}
				}
				lastchar = currentchar;
				lastcharwascursor = iscursor;
			}
			else
			{
				linebreakcnt--;
				linebreak = true;

				// Shouldn't be used:
				currentchar = ' ';
				currentfmt = 0;
			}

			if(col >= viewCols || last || linebreak)
			{
				char []nextRowTxt = new char[viewCols];
				char []nextRowFmt = new char[viewCols];
				for(int i = 0; i < viewCols; i++)
				{
					nextRowTxt[i] = ' ';
					nextRowFmt[i] = 0;
				}
				if(breaklast)
				{
					if(currentchar != ' ' || iscursor)
					{
						col--;
						int col0 = col;
						while(col >= 0 && rowTxt[col] != ' ')
						{
							col--;
						}
						if(col >= 0)
						{
String s = "";
							for(int i = col + 1, j = 0; i <= col0; i++)
							{
								nextRowTxt[j] = rowTxt[i];
								nextRowFmt[j] = rowFmt[i];
								j++;
s += nextRowTxt[j - 1];
							}
						}
						else
						{
							col = col0 + 1;	
						}
					}
					else
					{
						addchar = false;					
					}
				}

				// Check if this should be considered a blank line.
				// Note: could be improved by checking the actual contents if col > 0,
				// but not sure if it's needed.
				boolean blankline = (col == 0);				

				while(col < viewCols)
				{
					rowTxt[col] = ' ';
					rowFmt[col] = 0;
					col++;
				}

				if(pass == 0)
				{
					drawRowBackground(canvas, viewX, viewY, rowFmt, 0, viewCols, cursorindex);
				}
				else
				{
					int viewYF = viewY + squareSizeY - ybackgroffs;

					drawRowForeground(canvas, viewX, viewYF, rowTxt, rowFmt, 0, viewCols, cursorindex);
				}
				cursorindex = -1;

				// TEMP
				String rowTxtTmp = new String(rowTxt);
				displayedLines++;

				if(!blankline)
				{
					// For displayedLinesOnReformat, we don't count empty lines at the end.
					displayedLinesOnReformat = displayedLines;	
				}
				
				System.arraycopy(nextRowTxt, 0, rowTxt, 0, viewCols);
				System.arraycopy(nextRowFmt, 0, rowFmt, 0, viewCols);

				col = 0;
				while(col < viewCols && rowTxt[col] != ' ')
				{
					col++;
				}
				viewY += squareSizeY;
			}
			if(last)
			{
				break;
			}
			if(addchar)
			{
				if(iscursor)
				{
					cursorindex = col;
				}
				rowTxt[col] = currentchar;
				rowFmt[col] = currentfmt;
				col++;
			}
		}
	}

	// This can be enabled to test some text alignment stuff:
	/*
	String s = "123456789012345678901234567890123456789012345678901234567890";
	drawRowForeground(canvas, 0, squareSizeY, s.toCharArray(), terminal.fmtBuffer, 0, 60, -1);
	for(int x = 0; x < sizePixelsX; x += squareSizeX)
	{
		for(int y = 0; y < sizePixelsY; y += squareSizeY)
		{
			canvas.drawPoint(x - 0, y, textPaint); 
			canvas.drawPoint(x - 1, y, textPaint); 
			canvas.drawPoint(x - 0, y - 1, textPaint); 
			canvas.drawPoint(x - 1, y - 1, textPaint); 
		}
	}
	*/
	}


	protected void onDrawFixed(Canvas canvas)
	{
		int y;

		int rowView1 = 0;
		int rowView2 = Math.min(sizeY, terminal.numRows);
		int colView1 = 0;
		int colView2 = Math.min(sizeX, terminal.numColumns);

		Rect cliprect = new Rect();
		if(canvas.getClipBounds(cliprect))
		{
			colView1 = Math.max(computeViewColumnFromCoordX(cliprect.left), 0);
			colView2 = Math.min(computeViewColumnFromCoordX(cliprect.right + squareSizeX - 1), colView2);
			rowView1 = Math.max(computeViewRowFromCoordY(cliprect.top), 0);
			rowView2 = Math.min(computeViewRowFromCoordY(cliprect.bottom + squareSizeY - 1), rowView2);
		}

		y = computeViewCoordY(rowView1);
		for(int rowView = rowView1; rowView < rowView2; rowView++)
		{
			final int rowTerm = rowView + offsetY;
			final int buffOffs = rowTerm*terminal.numColumns + colView1 + offsetX; 
			int cursorIndex = -1;
			if(rowTerm == terminal.currentRow)
			{
				cursorIndex = terminal.currentColumn - colView1 - offsetX;
			}

			final int x = computeViewCoordX(colView1);

			drawRowBackground(canvas, x, y, terminal.fmtBuffer, buffOffs, colView2 - colView1, cursorIndex);
			
			y += squareSizeY;
		}

		int ybackgroffs = 2;
		y = squareSizeY + computeViewCoordY(rowView1) - ybackgroffs;
		for(int rowView = rowView1; rowView < rowView2; rowView++)
		{
			final int rowTerm = rowView + offsetY;
			final int buffOffs = rowTerm*terminal.numColumns + colView1 + offsetX; 
			int cursorIndex = -1;
			if(rowTerm == terminal.currentRow)
			{
				cursorIndex = terminal.currentColumn - colView1 - offsetX;
			}

			int x = computeViewCoordX(colView1);

			drawRowForeground(canvas, x, y, terminal.textBuffer, terminal.fmtBuffer, buffOffs, colView2 - colView1, cursorIndex);

			y += squareSizeY;
		}

		terminal.clearChange();

		if(drawCursor)
		{
			// Since we have drawn the cursor, we should probably register this current
			// position so that the next time we draw, we remember to erase the cursor
			// from its previous position.
			terminal.registerChange(terminal.currentColumn, terminal.currentRow);
		}

		displayedLinesOnReformat = 0;
	}
}
