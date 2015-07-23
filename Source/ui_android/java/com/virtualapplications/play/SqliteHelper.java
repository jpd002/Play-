package com.virtualapplications.play;

import android.net.Uri;
import android.provider.BaseColumns;

public class SqliteHelper {
	
	public SqliteHelper() {
	}
	
	public static final class Games implements BaseColumns {
		private Games() {
		}
		
		public static final String AUTHORITY = "virtualapplications.play.gamesdb";
		
		public static final String TABLE_NAME = "games";
		
		public static final Uri GAMES_URI = Uri.parse("content://" + AUTHORITY + "/" + TABLE_NAME);
		
		public static final String CONTENT_TYPE = "vnd.android.cursor.dir/vnd.virtualapplications.play";
		public static final String CONTENT_TYPE_ID = "vnd.android.cursor.item/vnd.virtualapplications.play";
		
		public static final String TABLE_ID = "_id";
		public static final String KEY_GAMEID = "GameID";
		public static final String KEY_TITLE = "GameTitle";
		public static final String KEY_OVERVIEW = "Overview";
		public static final String KEY_SERIAL = "Serial";
		public static final String KEY_BOXART = "Boxart";
	}
	
}