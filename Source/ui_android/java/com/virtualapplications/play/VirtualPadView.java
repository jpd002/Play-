package com.virtualapplications.play;

import android.app.*;
import android.content.*;
import android.graphics.*;
import android.util.*;
import android.view.*;

import java.io.StringReader;
import java.util.*;

import javax.xml.parsers.DocumentBuilder;
import javax.xml.parsers.DocumentBuilderFactory;

import org.w3c.dom.Document;
import org.w3c.dom.NamedNodeMap;
import org.w3c.dom.Node;
import org.w3c.dom.NodeList;

import org.xml.sax.InputSource;

public class VirtualPadView extends SurfaceView
{
	private ArrayList<VirtualPadItem> _items = new ArrayList<VirtualPadItem>();
	private HashMap<String, Bitmap> _itemBitmaps = new HashMap<String, Bitmap>();
	
	public VirtualPadView(Context context, AttributeSet attribs)
	{
		super(context, attribs);

		_itemBitmaps.put("select",      BitmapFactory.decodeResource(getResources(), R.drawable.select));
		_itemBitmaps.put("start",       BitmapFactory.decodeResource(getResources(), R.drawable.start));
		_itemBitmaps.put("up",          BitmapFactory.decodeResource(getResources(), R.drawable.up));
		_itemBitmaps.put("down",        BitmapFactory.decodeResource(getResources(), R.drawable.down));
		_itemBitmaps.put("left",        BitmapFactory.decodeResource(getResources(), R.drawable.left));
		_itemBitmaps.put("right",       BitmapFactory.decodeResource(getResources(), R.drawable.right));
		_itemBitmaps.put("triangle",    BitmapFactory.decodeResource(getResources(), R.drawable.triangle));
		_itemBitmaps.put("cross",       BitmapFactory.decodeResource(getResources(), R.drawable.cross));
		_itemBitmaps.put("square",      BitmapFactory.decodeResource(getResources(), R.drawable.square));
		_itemBitmaps.put("circle",      BitmapFactory.decodeResource(getResources(), R.drawable.circle));
		_itemBitmaps.put("lr",          BitmapFactory.decodeResource(getResources(), R.drawable.lr));
		_itemBitmaps.put("analogStick", BitmapFactory.decodeResource(getResources(), R.drawable.analogstick));
		
		setWillNotDraw(false);
	}
	
	private void createVirtualPad(int surfaceWidth, int surfaceHeight)
	{
		DisplayMetrics dm = new DisplayMetrics();
		((Activity)getContext()).getWindowManager().getDefaultDisplay().getMetrics(dm);
		float density = dm.density;
		float screenWidth = (float)surfaceWidth / density;
		float screenHeight = (float)surfaceHeight / density;

		_items.clear();

		String padItemsText = InputManager.getVirtualPadItems(screenWidth, screenHeight);
		DocumentBuilderFactory dbf = DocumentBuilderFactory.newInstance();
		try
		{
			DocumentBuilder db = dbf.newDocumentBuilder();
			InputSource is = new InputSource();
			is.setCharacterStream(new StringReader(padItemsText));
			Document document = db.parse(is);
			
			NodeList itemNodes = document.getElementsByTagName("Item");
			for(int i = 0; i < itemNodes.getLength(); i++)
			{
				Node itemNode = itemNodes.item(i);
				NamedNodeMap attributes = itemNode.getAttributes();
				boolean isAnalog = Boolean.parseBoolean(attributes.getNamedItem("isAnalog").getNodeValue());
				float x1 = Float.parseFloat(attributes.getNamedItem("x1").getNodeValue());
				float y1 = Float.parseFloat(attributes.getNamedItem("y1").getNodeValue());
				float x2 = Float.parseFloat(attributes.getNamedItem("x2").getNodeValue());
				float y2 = Float.parseFloat(attributes.getNamedItem("y2").getNodeValue());
				int code0 = Integer.parseInt(attributes.getNamedItem("code0").getNodeValue());
				int code1 = Integer.parseInt(attributes.getNamedItem("code1").getNodeValue());
				String caption = attributes.getNamedItem("caption").getNodeValue();
				String imageName = attributes.getNamedItem("imageName").getNodeValue();
				if(!_itemBitmaps.containsKey(imageName))
				{
					throw new Exception("Invalid image name.");
				}
				Bitmap bitmap = _itemBitmaps.get(imageName);
				RectF itemRect = new RectF(x1 * density, y1 * density, x2 * density, y2 * density);
				if(isAnalog)
				{
					_items.add(new VirtualPadStick(itemRect, code0, code1, bitmap));
				}
				else
				{
					_items.add(new VirtualPadButton(itemRect, code0, bitmap, caption));
				}
			}
		}
		catch(Exception e)
		{
			Log.e(Constants.TAG, String.format("Failed to create virtual pad items: %s", e.getMessage()));
			_items.clear();
		}
	}
	
	@Override
	protected void onSizeChanged(int w, int h, int oldw, int oldh)
	{
		//Log.w(Constants.TAG, String.format("onSizeChanged - %d, %d, %d, %d", w, h, oldw, oldh));
		createVirtualPad(w, h);
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
