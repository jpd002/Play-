package com.virtualapplications.play;

import android.graphics.*;

class VirtualPadStick extends VirtualPadItem
{
	private int _valueAxisX;
	private int _valueAxisY;
	private Bitmap _bitmap;
	
	private PointF _pressPosition = new PointF();
	private PointF _offset = new PointF();
	
	public VirtualPadStick(RectF bounds, int valueAxisX, int valueAxisY, Bitmap bitmap)
	{
		super(bounds);
		_valueAxisX = valueAxisX;
		_valueAxisY = valueAxisY;
		_bitmap = bitmap;
	}
	
	@Override
	public void onPointerDown(float x, float y)
	{
		_pressPosition = new PointF(x, y);
		_offset = new PointF();
	}
	
	@Override
	public void onPointerUp()
	{
		_pressPosition = new PointF();
		_offset = new PointF();
		InputManager.setAxisState(_valueAxisX, 0);
		InputManager.setAxisState(_valueAxisY, 0);
	}
	
	@Override
	public void onPointerMove(float x, float y)
	{
		float radius = _bounds.width();
		_offset = new PointF(x - _pressPosition.x, y - _pressPosition.y);
		_offset.x = Math.min(_offset.x,  radius);
		_offset.x = Math.max(_offset.x, -radius);
		_offset.y = Math.min(_offset.y,  radius);
		_offset.y = Math.max(_offset.y, -radius);
		InputManager.setAxisState(_valueAxisX, _offset.x / radius);
		InputManager.setAxisState(_valueAxisY, _offset.y / radius);
	}
	
	@Override
	public void draw(Canvas canvas)
	{
		RectF offsetRect = new RectF(_bounds);
		offsetRect.offset(_offset.x, _offset.y);

		Paint paint = new Paint();
		paint.setAntiAlias(true);
		canvas.drawBitmap(_bitmap, null, offsetRect, paint);
	}
};
