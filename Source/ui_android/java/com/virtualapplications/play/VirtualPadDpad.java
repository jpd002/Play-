package com.virtualapplications.play;

import android.graphics.*;
import android.util.Log;

class VirtualPadDpad extends VirtualPadItem
{
	private final Bitmap _bitmap;

	public VirtualPadDpad(RectF bounds, Bitmap bitmap)
	{
		super(bounds);
		_bitmap = bitmap;
	}

	@Override
	public void onPointerDown(float x, float y)
	{
		updateButtonState(x, y);
	}

	@Override
	public void onPointerUp()
	{
		InputManager.setButtonState(InputManagerConstants.BUTTON_UP, false);
		InputManager.setButtonState(InputManagerConstants.BUTTON_DOWN, false);
		InputManager.setButtonState(InputManagerConstants.BUTTON_LEFT, false);
		InputManager.setButtonState(InputManagerConstants.BUTTON_RIGHT, false);
	}

	@Override
	public void onPointerMove(float x, float y)
	{
		updateButtonState(x, y);
	}

	@Override
	public void draw(Canvas canvas)
	{
		Paint paint = new Paint();
		paint.setAntiAlias(true);
		canvas.drawBitmap(_bitmap, null, _bounds, paint);
//		canvas.drawRect(_bounds, _paint);
//		_paint.setColor(Color.RED);
//		canvas.drawRect(_pressPosition.x - 20, _pressPosition.y - 20, _pressPosition.x + 20, _pressPosition.y + 20, _paint);
	}

	private void updateButtonState(float x, float y)
	{
		boolean upPressed = false;
		boolean downPressed = false;
		boolean leftPressed = false;
		boolean rightPressed = false;
		if(_bounds.contains(x, y))
		{
			float localX = _bounds.centerX() - x;
			float localY = _bounds.centerY() - y;
			double angle = Math.atan2(localY, localX);
			int quad = (int)(angle * 8 / Math.PI);
			//Log.w(Constants.TAG, String.format("Quad: %d", quad));
			switch(quad)
			{
			case 0:
				leftPressed = true;
				break;
			case 1:
			case 2:
				upPressed = true;
				leftPressed = true;
				break;
			case 3:
			case 4:
				upPressed = true;
				break;
			case 5:
			case 6:
				rightPressed = true;
				upPressed = true;
				break;
			case 7:
			case -7:
				rightPressed = true;
				break;
			case -6:
			case -5:
				rightPressed = true;
				downPressed = true;
				break;
			case -4:
			case -3:
				downPressed = true;
				break;
			case -2:
			case -1:
				downPressed = true;
				leftPressed = true;
				break;
			}
		}
		InputManager.setButtonState(InputManagerConstants.BUTTON_UP, upPressed);
		InputManager.setButtonState(InputManagerConstants.BUTTON_DOWN, downPressed);
		InputManager.setButtonState(InputManagerConstants.BUTTON_LEFT, leftPressed);
		InputManager.setButtonState(InputManagerConstants.BUTTON_RIGHT, rightPressed);
	}
}
