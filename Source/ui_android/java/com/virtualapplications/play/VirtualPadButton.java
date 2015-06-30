package com.virtualapplications.play;

import android.graphics.*;

class VirtualPadButton extends VirtualPadItem
{
	private String _caption;
	private Bitmap _bitmap;
	private int _value;
	private boolean _pressed = false;
	
	public VirtualPadButton(String caption, int value, RectF bounds, Bitmap bitmap)
	{
		super(bounds);
		_caption = caption;
		_bitmap = bitmap;
		_value = value;
	}
	
	@Override
	public void onPointerDown(float x, float y)
	{
		_pressed = true;
		InputManager.setButtonState(_value, true);
		super.onPointerDown(x, y);
	}
	
	@Override
	public void onPointerUp()
	{
		_pressed = false;
		InputManager.setButtonState(_value, false);
		super.onPointerUp();
	}
	
	@Override
	public void draw(Canvas canvas)
	{
		Paint paint = new Paint();
		paint.setAntiAlias(true);
		paint.setFilterBitmap(true);
		if(_pressed)
		{
			paint.setColorFilter(new LightingColorFilter(0xFFBBBBBB, 0x00000000));
		}
		canvas.drawBitmap(_bitmap, null, _bounds, paint);
	}
};
