package com.virtualapplications.play;

import android.content.ContentValues;
import android.content.Intent;
import android.graphics.Bitmap;
import android.net.Uri;
import android.os.Bundle;
import android.support.v7.app.AppCompatActivity;
import android.support.v7.widget.Toolbar;
import android.view.Menu;
import android.view.MenuItem;
import android.view.View;
import android.widget.TextView;

import com.virtualapplications.play.database.GameInfo;
import com.virtualapplications.play.database.IndexingDB;

import java.io.IOException;
import java.io.InputStream;

public class GameInfoEditActivity extends AppCompatActivity
{
	private final int SELECT_PHOTO = 1003;
	private Intent intent;
	private Bitmap selectedImage;
	private GameInfo gi;
	private boolean default_cover;
	private GamesAdapter.CoverViewHolder viewHolder;

	@Override
	protected void onCreate(Bundle savedInstanceState)
	{
		super.onCreate(savedInstanceState);
		setContentView(R.layout.activity_gameinfo_edit);
		ThemeManager.applyTheme(this);
		Toolbar tv = (Toolbar)findViewById(R.id.my_awesome_toolbar);
		setSupportActionBar(tv);

		intent = getIntent();
		gi = new GameInfo(this);

		viewHolder = new GamesAdapter.CoverViewHolder(findViewById(R.id.coverlayout));
		setupView();

		View cover = viewHolder.gameImageView;
		cover.setOnClickListener(new View.OnClickListener()
		{
			@Override
			public void onClick(View view)
			{
				Intent photoPickerIntent = new Intent(Intent.ACTION_PICK);
				photoPickerIntent.setType("image/*");
				startActivityForResult(photoPickerIntent, SELECT_PHOTO);
			}
		});

		View change_image = findViewById(R.id.button);
		change_image.setOnClickListener(new View.OnClickListener()
		{
			@Override
			public void onClick(View view)
			{
				Intent photoPickerIntent = new Intent(Intent.ACTION_PICK);
				photoPickerIntent.setType("image/*");
				startActivityForResult(photoPickerIntent, SELECT_PHOTO);
			}
		});

		View default_image = findViewById(R.id.button2);
		default_image.setOnClickListener(new View.OnClickListener()
		{
			@Override
			public void onClick(View view)
			{
				gi.removeBitmapFromMemCache(intent.getStringExtra("indexid"));
				gi.setCoverImage(intent.getStringExtra("indexid"), viewHolder, intent.getStringExtra("cover"), false, 0);
				default_cover = true;
			}
		});
	}

	private void setupView()
	{
		((TextView)findViewById(R.id.editText)).setText(intent.getStringExtra("title"));
		((TextView)findViewById(R.id.editText2)).setText(intent.getStringExtra("overview"));
		gi.setCoverImage(intent.getStringExtra("indexid"), viewHolder, intent.getStringExtra("cover"), 0);
		selectedImage = null;
		default_cover = false;
	}

	@Override
	protected void onActivityResult(int requestCode, int resultCode, Intent imageReturnedIntent)
	{
		super.onActivityResult(requestCode, resultCode, imageReturnedIntent);

		switch(requestCode)
		{
		case SELECT_PHOTO:
			if(resultCode == RESULT_OK)
			{
				try
				{
					final Uri imageUri = imageReturnedIntent.getData();
					InputStream inputStream = getContentResolver().openInputStream(imageUri);
					selectedImage = ImageUtils.getSampledImage(this, inputStream);
					default_cover = false;
					viewHolder.gameImageView.setImageBitmap(selectedImage);
					if(inputStream != null)
					{
						inputStream.close();
					}
				}
				catch(IOException e)
				{
					e.printStackTrace();
				}
			}
		}
	}

	@Override
	public boolean onCreateOptionsMenu(Menu menu)
	{
		// Inflate the menu; this adds items to the action bar if it is present.
		getMenuInflater().inflate(R.menu.editor, menu);
		return true;
	}

	@Override
	public boolean onOptionsItemSelected(MenuItem item)
	{

		switch(item.getItemId())
		{
		case R.id.action_save:
			IndexingDB GI = new IndexingDB(this);
			ContentValues values = new ContentValues();
			values.put(IndexingDB.KEY_GAMETITLE, ((TextView)findViewById(R.id.editText)).getText().toString());
			values.put(IndexingDB.KEY_OVERVIEW, ((TextView)findViewById(R.id.editText2)).getText().toString());
			if(intent.getStringExtra("cover") == null || intent.getStringExtra("cover").equals("404"))
			{
				values.put(IndexingDB.KEY_IMAGE, "200");
			}
			GI.updateIndex(values, IndexingDB.KEY_ID + "=?", new String[]{intent.getStringExtra("indexid")});
			GI.close();
			gi.removeBitmapFromMemCache(intent.getStringExtra("indexid"));
			if(!default_cover && selectedImage != null)
			{
				gi.removeBitmapFromMemCache(intent.getStringExtra("indexid"));
				gi.saveImage(intent.getStringExtra("indexid"), "-custom", selectedImage);
			}
			else if(default_cover)
			{
				gi.deleteImage(intent.getStringExtra("indexid"), "-custom");
			}
			setResult(RESULT_OK, intent);
			finish();
			break;
		case R.id.action_clear_cancel:
			setResult(RESULT_CANCELED);
			finish();
			break;
		case R.id.action_reset:
			setupView();
			break;
		}

		return super.onOptionsItemSelected(item);
	}
}
