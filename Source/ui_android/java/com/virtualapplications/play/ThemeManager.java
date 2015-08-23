package com.virtualapplications.play;

import android.app.Activity;
import android.content.res.TypedArray;
import android.os.Build;
import android.preference.PreferenceManager;
import android.support.v7.widget.Toolbar;
import android.view.WindowManager;

public class ThemeManager
{
	public static String THEME_SELECTION = "ui.theme_selection";
	
	public static void applyTheme(Activity activity) 
	{
		String positionString = PreferenceManager.getDefaultSharedPreferences(activity).getString(THEME_SELECTION, "1");
		int position = Integer.valueOf(positionString);
		int theme;
		int toolbarTheme;
		switch(position) 
		{
		case 0:
			toolbarTheme = R.color.action_bar_Yellow;
			theme = R.style.Yellow;
			break;
		default:
		case 1:
			toolbarTheme = R.color.action_bar_Blue;
			theme = R.style.Blue;
			break;
		case 2:
			toolbarTheme = R.color.action_bar_Pink;
			theme = R.style.Pink;
			break;
		case 3:
			toolbarTheme = R.color.action_bar_purple;
			theme = R.style.Purple;
			break;
		case 4:
			toolbarTheme = R.color.action_bar_Teal;
			theme = R.style.Teal;
			break;
		case 5:
			toolbarTheme = R.color.action_bar_Dark_Purple;
			theme = R.style.Dark_Purple;
			break;
		}
		Toolbar toolbar = (Toolbar)activity.findViewById(R.id.my_awesome_toolbar);
		if(toolbar != null)
		{
			toolbar.setBackgroundResource(toolbarTheme);
		}
		activity.getTheme().applyStyle(theme, true);
		if(Build.VERSION.SDK_INT >= Build.VERSION_CODES.LOLLIPOP) 
		{
			int color = getThemeColor(activity, R.attr.colorPrimaryDark);
			activity.getWindow().addFlags(WindowManager.LayoutParams.FLAG_DRAWS_SYSTEM_BAR_BACKGROUNDS);
			activity.getWindow().setStatusBarColor(color);
		}
	}
	
	static int getThemeColor(Activity activity, int attribute)
	{
		TypedArray a = activity.getTheme().obtainStyledAttributes(new int[] { attribute });
		int attributeColor = a.getColor(0, 0);
		a.recycle();
		return attributeColor;
	}
}
