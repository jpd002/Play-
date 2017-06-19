package com.virtualapplications.play;

import android.app.Activity;
import android.os.Build;
import android.preference.PreferenceManager;
import android.support.v7.widget.Toolbar;
import android.util.TypedValue;
import android.view.WindowManager;

public class ThemeManager
{
	public static String THEME_SELECTION = "ui.theme_selection";
	
	public static void applyTheme(Activity activity) 
	{
		String positionString = PreferenceManager.getDefaultSharedPreferences(activity).getString(THEME_SELECTION, "1");
		int position = Integer.valueOf(positionString);
		int theme;
		switch(position)
		{
		case 0:
			theme = R.style.Amber;
			break;
		default:
		case 1:
			theme = R.style.Blue;
			break;
		case 2:
			theme = R.style.Pink;
			break;
		case 3:
			theme = R.style.Purple;
			break;
		case 4:
			theme = R.style.Teal;
			break;
		case 5:
			theme = R.style.Dark_Purple;
			break;
		case 6:
			theme = R.style.Green;
			break;
		}
		activity.getTheme().applyStyle(theme, true);
		Toolbar toolbar = (Toolbar)activity.findViewById(R.id.my_awesome_toolbar);
		if(toolbar != null)
		{
			int color = getThemeColor(activity, R.attr.colorPrimary);
			toolbar.setBackgroundColor(color);
		}
		if(Build.VERSION.SDK_INT >= Build.VERSION_CODES.LOLLIPOP)
		{
			int color = getThemeColor(activity, R.attr.colorPrimaryDark);
			activity.getWindow().addFlags(WindowManager.LayoutParams.FLAG_DRAWS_SYSTEM_BAR_BACKGROUNDS);
			activity.getWindow().setStatusBarColor(color);
		}
	}

	static int getThemeColor(Activity activity, int attribute)
	{
		TypedValue typedValue = new TypedValue();
		activity.getTheme().resolveAttribute(attribute, typedValue, true);
		int color = typedValue.data;
		return color;
	}
}
