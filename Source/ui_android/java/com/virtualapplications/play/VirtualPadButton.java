package com.virtualapplications.play;

import android.graphics.*;
import android.text.*;

class VirtualPadButton extends VirtualPadItem
{
	private int _value;
	private Bitmap _bitmap;
	private String _caption;

	private boolean _pressed = false;
	
	public VirtualPadButton(RectF bounds, int value, Bitmap bitmap)
	{
		super(bounds);
		_value = value;
		_bitmap = bitmap;
	}

	public VirtualPadButton(RectF bounds, int value, Bitmap bitmap, String caption)
	{
		this(bounds, value, bitmap);
		_caption = caption;
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

		if(!TextUtils.isEmpty(_caption))
		{
			paint.setColor(Color.WHITE);
			paint.setTextAlign(Paint.Align.CENTER);
			paint.setTextSize(40);
			float textOffset = (paint.descent() + paint.ascent()) / 2;
			canvas.drawText(_caption, _bounds.centerX(), _bounds.centerY() - textOffset, paint);
		}
	}
};
