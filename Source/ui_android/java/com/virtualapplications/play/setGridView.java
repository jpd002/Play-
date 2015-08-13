package com.virtualapplications.play;

import android.app.Activity;
import android.app.ProgressDialog;
import android.content.Context;
import android.graphics.Bitmap;
import android.os.AsyncTask;
import android.view.LayoutInflater;
import android.view.View;
import android.view.ViewGroup;
import android.widget.ArrayAdapter;
import android.widget.GridView;
import android.widget.ImageView;
import android.widget.TextView;

import com.virtualapplications.play.database.GameInfo;

import java.io.File;
import java.util.ArrayList;
import java.util.List;

import static com.virtualapplications.play.MainActivity.getNavigationBarSize;

public class setGridView extends AsyncTask<String, Integer, List<GameInfoStruct>> {

    private final List<File> images;
    private int[] measures;
    private final GameInfo gameInfo;
    public Context mContext;
    private int padding;
    private ProgressDialog progDialog;

    public setGridView(Activity mContext, List<File> images, GameInfo gameInfo) {
        this.mContext = mContext;
        this.images = images;
        this.gameInfo = gameInfo;
    }

    protected void onPreExecute() {
        progDialog = ProgressDialog.show(mContext,
                mContext.getString(R.string.setting_view),
                mContext.getString(R.string.please_wait), true);
        int mWidth = 0;
        int mHeight = 0;
        if (MainActivity.isConfigured){
            View v = LayoutInflater.from(mContext).inflate(R.layout.game_list_item, null, false);
            v.measure(0, 0);
            mWidth = v.findViewById(R.id.game_icon).getMeasuredWidth();
            mHeight = v.findViewById(R.id.game_icon).getMeasuredHeight();
        }
        measures = new int[]{mHeight, mWidth};
        padding = getNavigationBarSize(mContext).y;
    }

    @Override
    protected List<GameInfoStruct> doInBackground(String... params) {

        List<GameInfoStruct> games = new ArrayList<>();
        for (File game : images){
            GameInfoStruct tmp;
            if (!MainActivity.isConfigured || MainActivity.IsLoadableExecutableFileName(game.getName())) {
                tmp = new GameInfoStruct(game);
            } else {
                tmp = gameInfo.getGameInfo(game,measures);
                tmp.setFile(game);
                if (tmp.getFrontLink() != null && !tmp.getFrontLink().equals("404")) {
                    Bitmap cover = gameInfo.getImage(tmp.getID(), measures, tmp.getFrontLink());
                    tmp.setFrontCover(cover);
                }
            }

            games.add(tmp);
        }
        return games;
    }

    @Override
    protected void onPostExecute(List<GameInfoStruct> games) {
        GridView gameGrid = (GridView) ((MainActivity)mContext).findViewById(R.id.game_grid);
        if (gameGrid != null && gameGrid.isShown()) {
            gameGrid.setAdapter(null);
        }

        GamesAdapter adapter = new GamesAdapter(mContext, MainActivity.isConfigured ? R.layout.game_list_item : R.layout.file_list_item, games, padding);
		/*
		gameGrid.setNumColumns(-1);
		-1 = autofit
		or set a number if you like
		 */
        if (MainActivity.isConfigured){
            gameGrid.setColumnWidth((int) mContext.getResources().getDimension(R.dimen.cover_width));
        }
        gameGrid.setAdapter(adapter);
        gameGrid.invalidate();
        if (progDialog != null && progDialog.isShowing()) {
            progDialog.dismiss();
        }
    }

    public class GamesAdapter extends ArrayAdapter<GameInfoStruct> {

        private final int layoutid;
        private final int padding;
        private List<GameInfoStruct> games;
        private int original_bottom_pad;

        public GamesAdapter(Context context, int ResourceId, List<GameInfoStruct> images, int padding) {
            super(context, ResourceId, images);
            this.games = images;
            this.layoutid = ResourceId;
            this.padding = padding;
        }

        public int getCount() {
            return games.size();
        }

        public GameInfoStruct getItem(int position) {
            return games.get(position);
        }

        public long getItemId(int position) {
            return position;
        }

        @Override
        public View getView(final int position, View convertView, final ViewGroup parent) {
            View v = convertView;
            if (v == null) {
                LayoutInflater vi = (LayoutInflater) mContext.getSystemService(Context.LAYOUT_INFLATER_SERVICE);
                v = vi.inflate(layoutid, null);
            }

            final GameInfoStruct game = games.get(position);
            if (game != null) {
                createListItem(game, v);
            }
            if (original_bottom_pad == 0){
                original_bottom_pad = v.getPaddingBottom();
            }
            if (position == games.size() - 1){
                v.setPadding(
                        v.getPaddingLeft(),
                        v.getPaddingTop(),
                        v.getPaddingRight(),
                        original_bottom_pad + padding);
            } else {
                v.setPadding(
                        v.getPaddingLeft(),
                        v.getPaddingTop(),
                        v.getPaddingRight(),
                        original_bottom_pad);
            }
            return v;
        }
        private View createListItem(final GameInfoStruct gameStats, final View childview) {
            if (!MainActivity.isConfigured) {

                ((TextView) childview.findViewById(R.id.game_text)).setText(gameStats.getFile().getName());

                childview.findViewById(R.id.childview).setOnClickListener(new View.OnClickListener() {
                    public void onClick(View view) {
                        MainActivity.setCurrentDirectory(gameStats.getFile().getPath().substring(0,
                                gameStats.getFile().getPath().lastIndexOf(File.separator)));
                        MainActivity.isConfigured = true;
                        MainActivity.prepareFileListView(false, null, mContext);
                        return;
                    }
                });

                return childview;
            } else {

                ((TextView) childview.findViewById(R.id.game_text)).setText(gameStats.getTitleName());
                ImageView preview = (ImageView) childview.findViewById(R.id.game_icon);

                childview.findViewById(R.id.childview).setOnLongClickListener(
                        gameInfo.configureLongClick(gameStats.getTitleName(), gameStats.getDescription(), gameStats.getFile()));

                if (gameStats.getFrontLink() != null && !gameStats.getFrontLink().equals("404")) {
                    preview.setImageBitmap(gameStats.getFrontCover());
                    preview.setScaleType(ImageView.ScaleType.CENTER_INSIDE);
                    ((TextView) childview.findViewById(R.id.game_text)).setVisibility(View.GONE);
                } else {
                    preview.setImageResource(R.drawable.boxart);
                    preview.setScaleType(ImageView.ScaleType.CENTER_INSIDE);
                    ((TextView) childview.findViewById(R.id.game_text)).setVisibility(View.VISIBLE);
                }

                childview.findViewById(R.id.childview).setOnClickListener(new View.OnClickListener() {
                    public void onClick(View view) {
                        MainActivity.launchGame(gameStats.getFile());
                        return;
                    }
                });

                return childview;
            }
        }

    }
}
