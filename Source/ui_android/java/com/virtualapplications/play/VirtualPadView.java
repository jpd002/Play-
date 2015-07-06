package com.virtualapplications.play;

import android.app.*;
import android.content.*;
import android.graphics.*;
import android.util.*;
import android.view.*;
import java.util.*;

public class VirtualPadView extends SurfaceView
{
	private ArrayList<VirtualPadItem> _items = new ArrayList<VirtualPadItem>();

	private Bitmap select = BitmapFactory.decodeResource(getResources(), R.drawable.select);
	private Bitmap start = BitmapFactory.decodeResource(getResources(), R.drawable.start);
	private Bitmap up = BitmapFactory.decodeResource(getResources(), R.drawable.up);
	private Bitmap down = BitmapFactory.decodeResource(getResources(), R.drawable.down);
	private Bitmap left = BitmapFactory.decodeResource(getResources(), R.drawable.left);
	private Bitmap right = BitmapFactory.decodeResource(getResources(), R.drawable.right);
	private Bitmap triangle = BitmapFactory.decodeResource(getResources(), R.drawable.triangle);
	private Bitmap cross = BitmapFactory.decodeResource(getResources(), R.drawable.cross);
	private Bitmap square = BitmapFactory.decodeResource(getResources(), R.drawable.square);
	private Bitmap circle = BitmapFactory.decodeResource(getResources(), R.drawable.circle);
	private Bitmap lr = BitmapFactory.decodeResource(getResources(), R.drawable.lr);
	private Bitmap analogStick = BitmapFactory.decodeResource(getResources(), R.drawable.analogstick);
	
	public VirtualPadView(Context context, AttributeSet attribs)
	{
		super(context, attribs);

		setWillNotDraw(false);
	}
	
	@Override
	protected void onSizeChanged(int w, int h, int oldw, int oldh)
	{
		Log.w(Constants.TAG, String.format("onSizeChanged - %d, %d, %d, %d", w, h, oldw, oldh));

		float analogStickSize = 192;
		float lrButtonWidth = 256;
		float lrButtonHeight = 128;
		float margin = 64;
		
		float dpadPosX = margin;
		float dpadPosY = h - 384 - margin;
		float actionPadPosX = w - 384 - margin;
		float actionPadPosY = h - 384 - margin;
		float startSelPadPosX = (w - 384) / 2;
		float startSelPadPosY = h - 64 - margin;
		float leftButtonsPosX = margin;
		float leftButtonsPosY = margin;
		float rightButtonsPosX = w - (margin + lrButtonWidth);
		float rightButtonsPosY = margin;
		float leftAnalogStickPosX = dpadPosX + 384 + analogStickSize;
		float rightAnalogStickPosX = actionPadPosX - (analogStickSize * 2);
		float analogStickPosY = h - 384 - margin;
		
		_items.clear();

		_items.add(new VirtualPadButton(
				new RectF(startSelPadPosX + 0, startSelPadPosY + 0, startSelPadPosX + 128, startSelPadPosY + 64),
				InputManagerConstants.BUTTON_SELECT, select));
		_items.add(new VirtualPadButton(
				new RectF(startSelPadPosX + 256, startSelPadPosY + 0, startSelPadPosX + 384, startSelPadPosY + 64),
				InputManagerConstants.BUTTON_START, start));

		_items.add(new VirtualPadButton(
				new RectF(dpadPosX + 128, dpadPosY + 0, dpadPosX + 256, dpadPosY + 192),
				InputManagerConstants.BUTTON_UP, up));
		_items.add(new VirtualPadButton(
				new RectF(dpadPosX + 128, dpadPosY + 192, dpadPosX + 256, dpadPosY + 384),
				InputManagerConstants.BUTTON_DOWN, down));
		_items.add(new VirtualPadButton(
				new RectF(dpadPosX + 0, dpadPosY + 128, dpadPosX + 192, dpadPosY + 256),
				InputManagerConstants.BUTTON_LEFT, left));
		_items.add(new VirtualPadButton(
				new RectF(dpadPosX + 192, dpadPosY + 128, dpadPosX + 384, dpadPosY + 256),
				InputManagerConstants.BUTTON_RIGHT, right));

		_items.add(new VirtualPadButton(
				new RectF(leftButtonsPosX, leftButtonsPosY, leftButtonsPosX + lrButtonWidth, leftButtonsPosY + lrButtonHeight),
				InputManagerConstants.BUTTON_L2, lr, "L2"));
		_items.add(new VirtualPadButton(
				new RectF(leftButtonsPosX, leftButtonsPosY + lrButtonHeight, leftButtonsPosX + lrButtonWidth, leftButtonsPosY + lrButtonHeight * 2),
				InputManagerConstants.BUTTON_L1, lr, "L1"));
		_items.add(new VirtualPadButton(
				new RectF(rightButtonsPosX, rightButtonsPosY, rightButtonsPosX + lrButtonWidth, rightButtonsPosY + lrButtonHeight),
				InputManagerConstants.BUTTON_R2, lr, "R2"));
		_items.add(new VirtualPadButton(
				new RectF(rightButtonsPosX, rightButtonsPosY + lrButtonHeight, rightButtonsPosX + lrButtonWidth, rightButtonsPosY + lrButtonHeight * 2),
				InputManagerConstants.BUTTON_R1, lr, "R1"));
				
		_items.add(new VirtualPadButton(
				new RectF(actionPadPosX + 128, actionPadPosY + 0, actionPadPosX + 256, actionPadPosY + 128),
				InputManagerConstants.BUTTON_TRIANGLE, triangle));
		_items.add(new VirtualPadButton(
				new RectF(actionPadPosX + 128, actionPadPosY + 256, actionPadPosX + 256, actionPadPosY + 384),
				InputManagerConstants.BUTTON_CROSS, cross));
		_items.add(new VirtualPadButton(
				new RectF(actionPadPosX + 0, actionPadPosY + 128, actionPadPosX + 128, actionPadPosY + 256),
				InputManagerConstants.BUTTON_SQUARE, square));
		_items.add(new VirtualPadButton(
				new RectF(actionPadPosX + 256, actionPadPosY + 128, actionPadPosX + 384, actionPadPosY + 256),
				InputManagerConstants.BUTTON_CIRCLE, circle));

		_items.add(new VirtualPadStick(
				new RectF(leftAnalogStickPosX, analogStickPosY, leftAnalogStickPosX + analogStickSize, analogStickPosY + analogStickSize),
				InputManagerConstants.ANALOG_LEFT_X, InputManagerConstants.ANALOG_LEFT_Y, analogStick));
		_items.add(new VirtualPadStick(
				new RectF(rightAnalogStickPosX, analogStickPosY, rightAnalogStickPosX + analogStickSize, analogStickPosY + analogStickSize),
				InputManagerConstants.ANALOG_RIGHT_X, InputManagerConstants.ANALOG_RIGHT_Y, analogStick));

		postInvalidate();
	}
	
	@Override
	public void draw(Canvas canvas)
	{
		super.draw(canvas);
		for(VirtualPadItem item : _items)
		{
			item.draw(canvas);
		}
	}
	
	@Override
	public boolean onTouchEvent(final MotionEvent event)
	{
		int action = event.getActionMasked();
		if(action == MotionEvent.ACTION_DOWN || action == MotionEvent.ACTION_POINTER_DOWN)
		{
			int pointerIndex = event.getActionIndex();
			int pointerId = event.getPointerId(pointerIndex);
			float x = event.getX(pointerIndex);
			float y = event.getY(pointerIndex);
			for(VirtualPadItem item : _items)
			{
				RectF bounds = item.getBounds();
				if(bounds.contains(x, y))
				{
					item.setPointerId(pointerId);
					item.onPointerDown(x, y);
					postInvalidate();
					break;
				}
			}
		}
		else if(action == MotionEvent.ACTION_MOVE)
		{
			int pointerCount = event.getPointerCount();
			for(int i = 0; i < pointerCount; i++)
			{
				int pointerId = event.getPointerId(i);
				float x = event.getX(i);
				float y = event.getY(i);
				for(VirtualPadItem item : _items)
				{
					if(item.getPointerId() == pointerId)
					{
						item.onPointerMove(x, y);
						postInvalidate();
					}
				}
			}
		}
		else if(action == MotionEvent.ACTION_UP || action == MotionEvent.ACTION_POINTER_UP)
		{
			int pointerIndex = event.getActionIndex();
			int pointerId = event.getPointerId(pointerIndex);
			for(VirtualPadItem item : _items)
			{
				if(item.getPointerId() == pointerId)
				{
					item.onPointerUp();
					item.setPointerId(-1);
					postInvalidate();
					break;
				}
			}
		}
		return true;
	}
};
