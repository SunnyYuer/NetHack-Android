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

public class NetHackTiledView extends NetHackView
{
	private boolean drawCursor = true;
	private boolean whiteBackgroundMode = false;

	public Paint cursorPaint;
	public Paint backgroundPaint;

	public Bitmap tileBitmap;

	void setBitmap(Bitmap bm, int tilesizex, int tilesizey, int defaultzoompercentage)
	{
		tileBitmap = bm;

		tileSizeX = tilesizex;
		tileSizeY = tilesizey;

		tilesPerRow = bm.getWidth()/tilesizex;
		
		zoomPercentage = defaultzoompercentage;

		updateZoom();
	}

	public void updateZoom()
	{
		squareSizeX = (tileSizeX*zoomPercentage)/100;
		squareSizeY = (tileSizeY*zoomPercentage)/100;
	}

	int tilesPerRow = 1;
	int tileSizeX;
	int tileSizeY;

	public int zoomPercentage = 100;

	Paint bitmapPaint;
	
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

	public NetHackTiledView(Context context)
	{
		super(context);

		cursorPaint = new Paint();
		cursorPaint.setColor(Color.argb(0xff, 0xc0, 0xc0, 0xc0));

		backgroundPaint = new Paint();
		backgroundPaint.setColor(Color.argb(0xff, 0x00, 0x00, 0x00));

/* TEMP */
		int width = 80;
		int height = 24;		// 26
		terminal = new NetHackTerminalState(width, height);

		bitmapPaint = new Paint();
//		bitmapPaint.setAntiAlias(true);
		bitmapPaint.setAntiAlias(false);

		offsetX = 0;
		offsetY = 0;
		sizeX = terminal.numColumns;
		sizeY = terminal.numRows;

		computeSizePixels();
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
			backgroundPaint.setColor(Color.argb(0xff, 0xff, 0xff, 0xff));
		}
		else
		{
			setBackgroundColor(0xff000000);
			backgroundPaint.setColor(Color.argb(0xff, 0x00, 0x00, 0x00));
		}
	}

	protected void drawRow(Canvas canvas,
			int x, int y,
			final char []txtBuffer,
			final int buffOffs, final int numChars, /*final*/ int cursorIndex)
	{
		if(tileBitmap == null)
		{
			return;	
		}

		int x0 = x;
		for(int index = 0; index < numChars; x += squareSizeX, index++)
		{
			int buffIndex = buffOffs + index;
			if(buffIndex >= 0 && buffIndex < txtBuffer.length)
			{
				char c = txtBuffer[buffIndex];
				if(c >= 0x100)
				{
					int tileindex = c - 0x100;
					int bitmapcharwidth = tileSizeX;
					int bitmapsquareSizeY = tileSizeY;

					int tilex = tileindex % tilesPerRow;
					int tiley = tileindex/tilesPerRow;
					
					canvas.drawBitmap(tileBitmap, new Rect(tilex*bitmapcharwidth, tiley*bitmapsquareSizeY, (tilex + 1)*bitmapcharwidth, (tiley + 1)*bitmapsquareSizeY), new Rect(x, y, x + squareSizeX, y + squareSizeY), bitmapPaint);
				}
				else
				{
					if(whiteBackgroundMode)
					{
						// Not sure if we should also do this if the background is black -
						// it doesn't seem to be necessary, but I'm a little unclear on what
						// would ensure that the background is black in that case.
						Rect destRect = new Rect(x, y, x + squareSizeX, y + squareSizeY);
						canvas.drawRect(destRect, backgroundPaint);

						// TODO: Also, we should probably consider just filling the redrawn area
						// with a single rectangle, rather than doing it for each tile. Would probably
						// be faster, and possibly help (or hurt?) some artifacts I was seeing.
					}
				}
			}
		}

		if(cursorIndex >= 0)
		{
			int x1 = x0 + squareSizeX*cursorIndex;
			int y1 = y;
			int x2 = x1 + squareSizeX - 1;
			int y2 = y1 + squareSizeY - 1;
			int offsx = squareSizeX/4;
			int offsy = squareSizeY/4;
			
			canvas.drawLine(x1, y1, x1 + offsx, y1, cursorPaint);
			canvas.drawLine(x1, y1, x1, y1 + offsy, cursorPaint);

			canvas.drawLine(x2, y1, x2 - offsx, y1, cursorPaint);
			canvas.drawLine(x2, y1, x2, y1 + offsy, cursorPaint);

			canvas.drawLine(x1, y2, x1 + offsx, y2, cursorPaint);
			canvas.drawLine(x1, y2, x1, y2 - offsy, cursorPaint);

			canvas.drawLine(x2, y2, x2 - offsx, y2, cursorPaint);
			canvas.drawLine(x2, y2, x2, y2 - offsy, cursorPaint);
		}
	}


	protected void onDraw(Canvas canvas)
	{
		int y;

		pendingRedraw = false;

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

			int x = computeViewCoordX(colView1);

			drawRow(canvas, x, y, terminal.textBuffer, buffOffs, colView2 - colView1, cursorIndex);

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

	public void zoomIn()
	{
		if(zoomPercentage <= 400)
		{
			zoomPercentage *= 1.2f;
			zoomChanged();
		}
	}
	
	public void zoomOut()
	{
		if(zoomPercentage >= 20)
		{
			zoomPercentage *= (1.0/1.2f);
			zoomChanged();
		}
	}
}
