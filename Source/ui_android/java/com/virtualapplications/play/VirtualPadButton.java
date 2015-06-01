package com.virtualapplications.play;

import android.graphics.*;

class VirtualPadButton
{
	private int _pointerId;
	
	private String _caption;
	private RectF _bounds;
	private int _value;
	private boolean _pressed = false;
	private Bitmap _bitmap;
	
	public VirtualPadButton(String caption, int value, RectF bounds, Bitmap bitmap)
	{
		_caption = caption;
		_value = value;
		_bounds = bounds;
		_bitmap = bitmap;
	}
	
	public void setPointerId(int pointerId)
	{
		_pointerId = pointerId;
	}
	
	public int getPointerId()
	{
		return _pointerId;
	}
	
	public int getValue()
	{
		return _value;
	}
	
	public RectF getBounds()
	{
		return _bounds;
	}
	
	boolean getPressed()
	{
		return _pressed;
	}
	
	void setPressed(boolean pressed)
	{
		_pressed = pressed;
	}
	
	public void draw(Canvas canvas)
	{
		Paint paint = new Paint();
		paint.setAntiAlias(true);
		paint.setFilterBitmap(true);
		if (_pressed)
		{
			paint.setColorFilter(new LightingColorFilter(0xFFBBBBBB, 0x00000000));
		}
		canvas.drawBitmap(_bitmap, null, _bounds, paint);
	}
};
