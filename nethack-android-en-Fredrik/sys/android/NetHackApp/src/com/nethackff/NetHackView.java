package com.nethackff;

import android.content.Context;
import android.view.View;

public class NetHackView extends View
{
	public boolean pendingRedraw = false;

	NetHackView(Context context)
	{
		super(context);
	}

	public void computeSizePixels()
	{
		sizePixelsX = sizeX*squareSizeX + extraSizeX;
		sizePixelsY = sizeY*squareSizeY + extraSizeY;
	}

	public void setSizeX(int numColumns)
	{
		sizeX = numColumns;
	}
	public void setSizeXFromPixels(int pixelSizeX)
	{
		sizeX = pixelSizeX/squareSizeX;
	}
	public void setSizeY(int numRows)
	{
		sizeY = numRows;
	}
	public void setSizeYFromPixels(int pixelSizeY)
	{
		sizeY = pixelSizeY/squareSizeY;
	}
	public void initStateFromView()
	{
		terminal.init(sizeX, sizeY);
	}
	public int getSizeX()
	{
		return sizeX;
	}
	public int getSizeY()
	{
		return sizeY;
	}

	int computeCoordX(int column)
	{
		return squareSizeX*(column - offsetX);
	}

	int computeCoordY(int row)
	{
		return squareSizeY*(row - offsetY);
	}

	int computeColumnFromCoordX(int coordx)
	{
		return coordx/squareSizeX + offsetX;
	}

	int computeRowFromCoordY(int coordy)
	{
		return coordy/squareSizeY + offsetY;
	}

	int computeViewCoordX(int column)
	{
		return squareSizeX*column;
	}

	int computeViewCoordY(int row)
	{
		return squareSizeY*row;
	}

	int computeViewColumnFromCoordX(int coordx)
	{
		return coordx/squareSizeX;
	}

	int computeViewRowFromCoordY(int coordy)
	{
		return coordy/squareSizeY;
	}

	int computeViewColumnFromCoordXClamped(int coordx)
	{
		return Math.min(Math.max(coordx/squareSizeX, 0), sizeX - 1);
	}
	int computeViewColumnFromCoordYClamped(int coordy)
	{
		return Math.min(Math.max(coordy/squareSizeY, 0), sizeY - 1);
	}

	protected void onMeasure(int widthmeasurespec, int heightmeasurespec)
	{
		int minheight = getSuggestedMinimumHeight();
		int minwidth = getSuggestedMinimumWidth();

		int width, height;
		width = sizePixelsX;
		height = sizePixelsY;

		// This was done temporarily before, but hopefully there is no need for it now.
		//	if(reformatText)
		//	{
		//		height = height > 1000 ? height : 1000;	
		//	}

		if (width < minwidth)
		{
			width = minwidth;
		}
		if (height < minheight)
		{
			height = minheight;
		}

		int modex = MeasureSpec.getMode(widthmeasurespec);
		int modey = MeasureSpec.getMode(heightmeasurespec);
		if(modex == MeasureSpec.AT_MOST)
		{
			width = Math.min(MeasureSpec.getSize(widthmeasurespec), width);
		}
		else if(modex == MeasureSpec.EXACTLY)
		{
			width = MeasureSpec.getSize(widthmeasurespec);
		}
		if(modey == MeasureSpec.AT_MOST)
		{
			height = Math.min(MeasureSpec.getSize(heightmeasurespec), height);
		}
		else if(modey == MeasureSpec.EXACTLY)
		{
			height = MeasureSpec.getSize(heightmeasurespec);
		}
		setMeasuredDimension(width, height);
	}

	public void scrollToCenterAtPos(int centercolumn, int centerrow)
	{
		int cursorcenterx = centercolumn*squareSizeX + squareSizeX/2;
		int cursorcentery = centerrow*squareSizeY + squareSizeY/2;
		int newscrollx = cursorcenterx - getWidth()/2;
		int newscrolly = cursorcentery - getHeight()/2;

		int termx = squareSizeX*sizeX;
		int termy = squareSizeY*getNumDisplayedLines();

		int maxx = termx - getWidth();
		int maxy = termy - getHeight();		// Note: could be negative, so we do the max clamping first.
		if(newscrollx >= maxx)
		{
			newscrollx = maxx - 1;
		}
		if(newscrolly >= maxy)
		{
			newscrolly = maxy - 1;
		}
		if(newscrollx < 0)
		{
			newscrollx = 0;
		}
		if(newscrolly < 0)
		{
			newscrolly = 0;
		}

		scrollTo(newscrollx, newscrolly);
		desiredCenterPosX = cursorcenterx;
		desiredCenterPosY = cursorcentery;
	}

	public void scrollToCursor()
	{
		scrollToCenterAtPos(terminal.currentColumn, terminal.currentRow);
	}

	public int getNumDisplayedLines()
	{
		return sizeY;			
	}

	public void zoomIn()
	{
	}
	
	public void zoomOut()
	{
	}

	public void updateZoom()
	{
	}

	public void zoomChanged()
	{
		float centerXRel = desiredCenterPosX/squareSizeX;
		float centerYRel = desiredCenterPosY/squareSizeY;
		float oldSquareWidth = squareSizeX;
		float oldSquareHeight = squareSizeY;
		updateZoom();
		computeSizePixels();
		int newScrollX = (int)Math.round(centerXRel*squareSizeX - getWidth()*0.5f);
		int newScrollY = (int)Math.round(centerYRel*squareSizeY - getHeight()*0.5f);
		int unclampedScrollX = newScrollX;
		int unclampedScrollY = newScrollY;
		scrollToLimited1(newScrollX, newScrollY, false);
		desiredCenterPosX *= ((float)squareSizeX)/oldSquareWidth;
		desiredCenterPosY *= ((float)squareSizeY)/oldSquareHeight;
		invalidate();		
	}

	public void scrollToLimited1(int newscrollx, int newscrolly, boolean setnewdesiredcentertoactual)
	{
		int termx, termy;
		termx = squareSizeX*sizeX;
		termy = squareSizeY*sizeY;
		if(newscrollx < 0)
		{
			newscrollx = 0;
		}
		if(newscrolly < 0)
		{
			newscrolly = 0;
		}

		int maxx = termx - getWidth();
		int maxy = termy - getHeight();
		if(maxx < 0)
		{
			maxx = 0;
		}
		if(maxy < 0)
		{
			maxy = 0;
		}
		if(newscrollx >= maxx)
		{
			newscrollx = maxx - 1;
		}
		if(newscrolly >= maxy)
		{
			newscrolly = maxy - 1;
		}

		scrollTo(newscrollx, newscrolly);

		if(setnewdesiredcentertoactual)
		{
			desiredCenterPosX = newscrollx + getWidth()*0.5f;
			desiredCenterPosY = newscrolly + getHeight()*0.5f;
		}
	}

	int squareSizeY = 0;
	int squareSizeX = 0;

	public int extraSizeX = 0;
	public int extraSizeY = 0;

	public int sizeX;
	public int sizeY;
	public int sizePixelsX;
	public int sizePixelsY;

	public int offsetX = 0;
	public int offsetY = 0;

	public float desiredCenterPosX = 0;
	public float desiredCenterPosY = 0;

	NetHackTerminalState terminal;
};
