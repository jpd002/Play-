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
	
	private RectF createScaledRect(float x1, float y1, float x2, float y2, float scale)
	{
		return new RectF(x1 * scale, y1 * scale, x2 * scale, y2 * scale);
	}
	
	@Override
	protected void onSizeChanged(int w, int h, int oldw, int oldh)
	{
		//Log.w(Constants.TAG, String.format("onSizeChanged - %d, %d, %d, %d", w, h, oldw, oldh));

		DisplayMetrics dm = new DisplayMetrics();
		((Activity)getContext()).getWindowManager().getDefaultDisplay().getMetrics(dm);
		float density = dm.density;
		float screenWidth = (float)w / density;
		float screenHeight = (float)h / density;

		float uiScale = 1;
		if(screenHeight < 480)
		{
			uiScale = 1.5f;
		}
		else if(screenHeight < 768)
		{
			uiScale = 1.25f;
		}
		
		float analogStickSize = 96.0f / uiScale;
		float lrButtonWidth = 128.0f / uiScale;
		float lrButtonHeight = 64.0f / uiScale;
		float lr3ButtonSize = 64.0f / uiScale;
		float dpadButtonSize = 32.0f / uiScale;
		float margin = 32.0f / uiScale;
		float padButtonSize = 64.0f / uiScale;
		
		float dpadPosX = margin;
		float dpadPosY = screenHeight - (padButtonSize * 3) - margin;
		float actionPadPosX = screenWidth - (padButtonSize * 3) - margin;
		float actionPadPosY = screenHeight - (padButtonSize * 3) - margin;
		float startSelPadPosX = (screenWidth - (padButtonSize * 3)) / 2;
		float startSelPadPosY = screenHeight - padButtonSize - margin;
		float leftButtonsPosX = margin;
		float leftButtonsPosY = margin;
		float rightButtonsPosX = screenWidth - (margin + lrButtonWidth);
		float rightButtonsPosY = margin;
		float leftAnalogStickPosX = dpadPosX + (padButtonSize * 3) + analogStickSize;
		float rightAnalogStickPosX = actionPadPosX - (analogStickSize * 2);
		float analogStickPosY = screenHeight - (padButtonSize * 3) - margin;
		float l3ButtonPosX = startSelPadPosX - (lr3ButtonSize * 2);
		float r3ButtonPosX = startSelPadPosX + (padButtonSize * 3) + lr3ButtonSize;
		float lr3ButtonPosY = screenHeight - padButtonSize - margin;
		
		_items.clear();

		_items.add(new VirtualPadButton(
				createScaledRect(startSelPadPosX + padButtonSize * 0, startSelPadPosY + padButtonSize / 2, startSelPadPosX + padButtonSize * 1, startSelPadPosY + padButtonSize, density),
				InputManagerConstants.BUTTON_SELECT, select));
		_items.add(new VirtualPadButton(
				createScaledRect(startSelPadPosX + padButtonSize * 2, startSelPadPosY + padButtonSize / 2, startSelPadPosX + padButtonSize * 3, startSelPadPosY + padButtonSize, density),
				InputManagerConstants.BUTTON_START, start));

		_items.add(new VirtualPadButton(
				createScaledRect(dpadPosX + dpadButtonSize * 2, dpadPosY + dpadButtonSize * 0, dpadPosX + dpadButtonSize * 4, dpadPosY + dpadButtonSize * 3, density),
				InputManagerConstants.BUTTON_UP, up));
		_items.add(new VirtualPadButton(
				createScaledRect(dpadPosX + dpadButtonSize * 2, dpadPosY + dpadButtonSize * 3, dpadPosX + dpadButtonSize * 4, dpadPosY + dpadButtonSize * 6, density),
				InputManagerConstants.BUTTON_DOWN, down));
		_items.add(new VirtualPadButton(
				createScaledRect(dpadPosX + dpadButtonSize * 0, dpadPosY + dpadButtonSize * 2, dpadPosX + dpadButtonSize * 3, dpadPosY + dpadButtonSize * 4, density),
				InputManagerConstants.BUTTON_LEFT, left));
		_items.add(new VirtualPadButton(
				createScaledRect(dpadPosX + dpadButtonSize * 3, dpadPosY + dpadButtonSize * 2, dpadPosX + dpadButtonSize * 6, dpadPosY + dpadButtonSize * 4, density),
				InputManagerConstants.BUTTON_RIGHT, right));

		_items.add(new VirtualPadButton(
				createScaledRect(leftButtonsPosX, leftButtonsPosY, leftButtonsPosX + lrButtonWidth, leftButtonsPosY + lrButtonHeight, density),
				InputManagerConstants.BUTTON_L2, lr, "L2"));
		_items.add(new VirtualPadButton(
				createScaledRect(leftButtonsPosX, leftButtonsPosY + lrButtonHeight, leftButtonsPosX + lrButtonWidth, leftButtonsPosY + lrButtonHeight * 2, density),
				InputManagerConstants.BUTTON_L1, lr, "L1"));
		_items.add(new VirtualPadButton(
				createScaledRect(rightButtonsPosX, rightButtonsPosY, rightButtonsPosX + lrButtonWidth, rightButtonsPosY + lrButtonHeight, density),
				InputManagerConstants.BUTTON_R2, lr, "R2"));
		_items.add(new VirtualPadButton(
				createScaledRect(rightButtonsPosX, rightButtonsPosY + lrButtonHeight, rightButtonsPosX + lrButtonWidth, rightButtonsPosY + lrButtonHeight * 2, density),
				InputManagerConstants.BUTTON_R1, lr, "R1"));
				
		_items.add(new VirtualPadButton(
				createScaledRect(actionPadPosX + padButtonSize * 1, actionPadPosY + padButtonSize * 0, actionPadPosX + padButtonSize * 2, actionPadPosY + padButtonSize * 1, density),
				InputManagerConstants.BUTTON_TRIANGLE, triangle));
		_items.add(new VirtualPadButton(
				createScaledRect(actionPadPosX + padButtonSize * 1, actionPadPosY + padButtonSize * 2, actionPadPosX + padButtonSize * 2, actionPadPosY + padButtonSize * 3, density),
				InputManagerConstants.BUTTON_CROSS, cross));
		_items.add(new VirtualPadButton(
				createScaledRect(actionPadPosX + padButtonSize * 0, actionPadPosY + padButtonSize * 1, actionPadPosX + padButtonSize * 1, actionPadPosY + padButtonSize * 2, density),
				InputManagerConstants.BUTTON_SQUARE, square));
		_items.add(new VirtualPadButton(
				createScaledRect(actionPadPosX + padButtonSize * 2, actionPadPosY + padButtonSize * 1, actionPadPosX + padButtonSize * 3, actionPadPosY + padButtonSize * 2, density),
				InputManagerConstants.BUTTON_CIRCLE, circle));

		_items.add(new VirtualPadStick(
				createScaledRect(leftAnalogStickPosX, analogStickPosY, leftAnalogStickPosX + analogStickSize, analogStickPosY + analogStickSize, density),
				InputManagerConstants.ANALOG_LEFT_X, InputManagerConstants.ANALOG_LEFT_Y, analogStick));
		_items.add(new VirtualPadStick(
				createScaledRect(rightAnalogStickPosX, analogStickPosY, rightAnalogStickPosX + analogStickSize, analogStickPosY + analogStickSize, density),
				InputManagerConstants.ANALOG_RIGHT_X, InputManagerConstants.ANALOG_RIGHT_Y, analogStick));

		_items.add(new VirtualPadButton(
				createScaledRect(l3ButtonPosX, lr3ButtonPosY, l3ButtonPosX + lr3ButtonSize, lr3ButtonPosY + lr3ButtonSize, density),
				InputManagerConstants.BUTTON_L3, lr, "L3"));
		_items.add(new VirtualPadButton(
				createScaledRect(r3ButtonPosX, lr3ButtonPosY, r3ButtonPosX + lr3ButtonSize, lr3ButtonPosY + lr3ButtonSize, density),
				InputManagerConstants.BUTTON_R3, lr, "R3"));

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
