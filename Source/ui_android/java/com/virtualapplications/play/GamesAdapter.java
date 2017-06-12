package com.virtualapplications.play;

import android.content.Context;
import android.view.LayoutInflater;
import android.view.View;
import android.view.ViewGroup;
import android.widget.ArrayAdapter;
import android.widget.ImageView;
import android.widget.TextView;

import com.virtualapplications.play.database.GameInfo;

import java.util.List;

import static com.virtualapplications.play.MainActivity.launchGame;
import static com.virtualapplications.play.R.id.childview;

public class GamesAdapter extends ArrayAdapter<GameInfoStruct> {

    private final int layoutid;
    private List<GameInfoStruct> games;
    private GameInfo gameInfo;

    public static class CoverViewHolder{
        CoverViewHolder(View v)
        {
            this.childview = v;
            this.gameImageView = (ImageView) v.findViewById(R.id.game_icon);
            this.gameTextView = (TextView) v.findViewById(R.id.game_text);
            this.currentPositionView = (TextView) v.findViewById(R.id.currentPosition);
        }
        public View childview;
        public ImageView gameImageView;
        public TextView gameTextView;
        public TextView currentPositionView;
    }
    public GamesAdapter(Context context, int ResourceId, List<GameInfoStruct> images, GameInfo gameInfo) {
        super(context, ResourceId, images);
        this.games = images;
        this.layoutid = ResourceId;
        this.gameInfo = gameInfo;
    }

    public int getCount() {
        //return mThumbIds.length;
        return games.size();
    }

    public GameInfoStruct getItem(int position) {
        return games.get(position);
    }

    public long getItemId(int position) {
        return position;
    }

    @Override
    public View getView(int position, View v, ViewGroup parent) {
        CoverViewHolder viewHolder;
        if (v == null) {
            LayoutInflater vi = (LayoutInflater) getContext().getSystemService(Context.LAYOUT_INFLATER_SERVICE);
            v = vi.inflate(layoutid, null);
            viewHolder = new CoverViewHolder(v.findViewById(childview));
            v.setTag(viewHolder);
        }
        else
        {
            viewHolder = (CoverViewHolder) v.getTag();
            viewHolder.gameTextView.setText("");
            viewHolder.gameTextView.setVisibility(View.VISIBLE);
            viewHolder.gameImageView.setImageResource(R.drawable.boxart);
        }

        viewHolder.currentPositionView.setText(String.valueOf(position));

        final GameInfoStruct game = games.get(position);
        if (game != null) {
            createListItem(game, viewHolder, position);
        }
        return v;
    }

    private void createListItem(final GameInfoStruct game, final CoverViewHolder viewHolder, int pos) {
        viewHolder.gameTextView.setText(game.getTitleName());
        //If user has set values, then read them, if not read from database
        if (!game.isDescriptionEmptyNull() && game.getFrontLink() != null && !game.getFrontLink().equals("")) {
            viewHolder.childview.setOnLongClickListener(
                    gameInfo.configureLongClick(game));

            if (!game.getFrontLink().equals("404")) {
                gameInfo.setCoverImage(game.getGameID(), viewHolder, game.getFrontLink(), pos);
            } else {
                viewHolder.gameImageView.setImageResource(R.drawable.boxart);
            }
        } else if (VirtualMachineManager.IsLoadableExecutableFileName(game.getFile().getName())) {
            viewHolder.gameImageView.setImageResource(R.drawable.boxart);
            viewHolder.childview.setOnLongClickListener(null);
        } else {
            viewHolder.childview.setOnLongClickListener(null);
            // passing game, as to pass and use (if) any user defined values
            final GameInfoStruct gameStats = gameInfo.getGameInfo(game.getFile(), viewHolder, game, pos);

            if (gameStats != null) {
                games.set(pos, gameStats);
                viewHolder.childview.setOnLongClickListener(
                        gameInfo.configureLongClick(gameStats));

                if (gameStats.getFrontLink() != null && !gameStats.getFrontLink().equals("404")) {
                    gameInfo.setCoverImage(gameStats.getGameID(), viewHolder, gameStats.getFrontLink(), pos);
                }
            } else {
                viewHolder.gameImageView.setImageResource(R.drawable.boxart);
            }
        }



        viewHolder.childview.setOnClickListener(new View.OnClickListener() {
            public void onClick(View view) {
                launchGame(game, getContext());
                return;
            }
        });
    }

}
