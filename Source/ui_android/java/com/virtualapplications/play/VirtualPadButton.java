package com.virtualapplications.play;

import android.graphics.*;

class VirtualPadButton
{
	private String _caption;
	private RectF _bounds;
	private int _value;
	private boolean _pressed = false;
	
	public VirtualPadButton(String caption, int value, RectF bounds)
	{
		_caption = caption;
		_value = value;
		_bounds = bounds;
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
		{
			Paint squarePaint = new Paint();
			if(_pressed)
			{
				squarePaint.setARGB(255, 128, 128, 128);
			}
			else
			{
				squarePaint.setARGB(128, 128, 128, 128);
			}
			canvas.drawRect(_bounds, squarePaint);
		}

		{
			Paint textPaint = new Paint();
			textPaint.setColor(Color.WHITE);
			textPaint.setTextAlign(Paint.Align.CENTER);
			textPaint.setTextSize(40);
			float textOffset = (textPaint.descent() + textPaint.ascent()) / 2;
			canvas.drawText(_caption, _bounds.centerX(), _bounds.centerY() - textOffset, textPaint);
		}
	}
};
