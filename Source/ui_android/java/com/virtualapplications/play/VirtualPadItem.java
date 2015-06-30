package com.virtualapplications.play;

import android.graphics.*;

class VirtualPadItem
{
	private int _pointerId = -1;
	protected RectF _bounds;
	
	public VirtualPadItem(RectF bounds)
	{
		_bounds = bounds;
	}
	
	public void setPointerId(int pointerId)
	{
		_pointerId = pointerId;
	}
	
	public int getPointerId()
	{
		return _pointerId;
	}
	
	public RectF getBounds()
	{
		return _bounds;
	}
	
	public void onPointerDown(float x, float y)
	{
		
	}
	
	public void onPointerUp()
	{
		
	}
	
	public void onPointerMove(float x, float y)
	{
		
	}
	
	public void draw(Canvas canvas)
	{
		
	}
}
