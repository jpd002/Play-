package com.virtualapplications.play;

import android.content.ContentValues;
import android.content.Intent;
import android.graphics.Bitmap;
import android.graphics.BitmapFactory;
import android.net.Uri;
import android.support.v7.app.ActionBarActivity;
import android.os.Bundle;
import android.support.v7.widget.Toolbar;
import android.view.LayoutInflater;
import android.view.Menu;
import android.view.MenuItem;
import android.view.View;
import android.widget.ImageView;
import android.widget.TextView;

import com.virtualapplications.play.database.GameInfo;
import com.virtualapplications.play.database.IndexingDB;

import java.io.FileNotFoundException;

public class CoverEditActivity extends ActionBarActivity {

    private final int SELECT_PHOTO = 1003;
    private Intent intent;
    private Bitmap selectedImage;
    private GameInfo gi;
    private boolean default_cover;

    @Override
    protected void onCreate(Bundle savedInstanceState) {
        super.onCreate(savedInstanceState);
        setContentView(R.layout.activity_cover_edit);
        SettingsActivity.ChangeTheme(null, this);
        Toolbar tv = (Toolbar) findViewById(R.id.my_awesome_toolbar);
        setSupportActionBar(tv);

        intent = getIntent();
        gi = new GameInfo(this);

        setupView();

        View cover = findViewById(R.id.game_icon);
        cover.setOnClickListener(new View.OnClickListener() {

            @Override
            public void onClick(View view) {
                Intent photoPickerIntent = new Intent(Intent.ACTION_PICK);
                photoPickerIntent.setType("image/*");
                startActivityForResult(photoPickerIntent, SELECT_PHOTO);
            }
        });

        View change_image = findViewById(R.id.button);
        change_image.setOnClickListener(new View.OnClickListener() {

            @Override
            public void onClick(View view) {
                Intent photoPickerIntent = new Intent(Intent.ACTION_PICK);
                photoPickerIntent.setType("image/*");
                startActivityForResult(photoPickerIntent, SELECT_PHOTO);
            }
        });

        View default_image = findViewById(R.id.button2);
        default_image.setOnClickListener(new View.OnClickListener() {

            @Override
            public void onClick(View view) {
                gi.removeBitmapFromMemCache(intent.getStringExtra("gameid"));
                gi.getImage(intent.getStringExtra("gameid"), findViewById(R.id.coverlayout), intent.getStringExtra("cover"), false);
                default_cover = true;

            }
        });
    }

    private void setupView() {
        ((TextView)findViewById(R.id.editText)).setText(intent.getStringExtra("title"));
        ((TextView)findViewById(R.id.editText2)).setText(intent.getStringExtra("overview"));
        gi.getImage(intent.getStringExtra("gameid"), findViewById(R.id.coverlayout), intent.getStringExtra("cover"));
        selectedImage = null;
        default_cover =  false;
    }

    @Override
    protected void onActivityResult(int requestCode, int resultCode, Intent imageReturnedIntent) {
        super.onActivityResult(requestCode, resultCode, imageReturnedIntent);

        switch(requestCode) {
            case SELECT_PHOTO:
                if(resultCode == RESULT_OK){
                    try {
                        final Uri imageUri = imageReturnedIntent.getData();
                        BitmapFactory.Options options = new BitmapFactory.Options();
                        options.inJustDecodeBounds = true;
                        BitmapFactory.decodeStream(getContentResolver().openInputStream(imageUri), null, options);

                        options.inSampleSize = calculateInSampleSize(options);
                        options.inJustDecodeBounds = false;
                        selectedImage = BitmapFactory.decodeStream(getContentResolver().openInputStream(imageUri), null, options);
                        default_cover = false;
                        ((ImageView)findViewById(R.id.game_icon)).setImageBitmap(selectedImage);
                    } catch (FileNotFoundException e) {
                        e.printStackTrace();
                    }

                }
        }
    }

    private int calculateInSampleSize(BitmapFactory.Options options) {
        final int height = options.outHeight;
        final int width = options.outWidth;
        View v = LayoutInflater.from(this).inflate(R.layout.game_list_item, null, false);
        v.measure(0, 0);
        int reqWidth = v.findViewById(R.id.game_icon).getMeasuredWidth();
        int reqHeight = v.findViewById(R.id.game_icon).getMeasuredHeight();

        // TODO: Find a calculated width and height without ImageView
        int inSampleSize = 1;

        if (height > reqHeight || width > reqWidth) {

            final int halfHeight = height / 2;
            final int halfWidth = width / 2;

            while ((halfHeight / inSampleSize) > reqHeight
                    && (halfWidth / inSampleSize) > reqWidth) {
                inSampleSize *= 2;
            }
        }

        return inSampleSize;
    }

    @Override
    public boolean onCreateOptionsMenu(Menu menu) {
        // Inflate the menu; this adds items to the action bar if it is present.
        getMenuInflater().inflate(R.menu.editor, menu);
        return true;
    }

    @Override
    public boolean onOptionsItemSelected(MenuItem item) {

        switch (item.getItemId()) {
            case R.id.action_save:
                IndexingDB GI = new IndexingDB(this);
                ContentValues values = new ContentValues();
                values.put(IndexingDB.KEY_GAMETITLE, ((TextView) findViewById(R.id.editText)).getText().toString());
                values.put(IndexingDB.KEY_OVERVIEW, ((TextView) findViewById(R.id.editText2)).getText().toString());
                GI.updateIndex(values, IndexingDB.KEY_ID + "=?", new String[]{intent.getStringExtra("indexid")});
                if (intent.getStringExtra("cover") == null || intent.getStringExtra("cover").equals("404")){
                    values.put(IndexingDB.KEY_IMAGE, "200");
                }
                gi.removeBitmapFromMemCache(intent.getStringExtra("gameid"));
                if (!default_cover && selectedImage != null){
                    gi.removeBitmapFromMemCache(intent.getStringExtra("gameid"));
                    gi.saveImage(intent.getStringExtra("gameid") , "-custom", selectedImage);
                } else if (default_cover) {
                    gi.deleteImage(intent.getStringExtra("gameid") , "-custom");
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
