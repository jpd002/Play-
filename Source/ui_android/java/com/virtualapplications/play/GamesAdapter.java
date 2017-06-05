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

class GamesAdapter extends ArrayAdapter<GameInfoStruct> {

    private final int layoutid;
    private List<GameInfoStruct> games;
    private GameInfo gameInfo;

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
    public View getView(int position, View convertView, ViewGroup parent) {
        View v = convertView;
        if (v == null) {
            LayoutInflater vi = (LayoutInflater) this.getContext().getSystemService(Context.LAYOUT_INFLATER_SERVICE);
            v = vi.inflate(layoutid, null);
        }
        else
        {
            ((TextView) v.findViewById(R.id.game_text)).setText("");
            ((TextView) v.findViewById(R.id.game_text)).setVisibility(View.VISIBLE);
            ImageView preview = (ImageView) v.findViewById(R.id.game_icon);
            preview.setImageResource(R.drawable.boxart);

        }

        ((TextView) v.findViewById(R.id.currentPosition)).setText(String.valueOf(position));

        final GameInfoStruct game = games.get(position);
        if (game != null) {
            createListItem(game, v, position);
        }
        return v;
    }

    private View createListItem(final GameInfoStruct game, final View childview, int pos) {
        ((TextView) childview.findViewById(R.id.game_text)).setText(game.getTitleName());
        //If user has set values, then read them, if not read from database
        if (!game.isDescriptionEmptyNull() && game.getFrontLink() != null && !game.getFrontLink().equals("")) {
            childview.findViewById(R.id.childview).setOnLongClickListener(
                    gameInfo.configureLongClick(game));

            if (!game.getFrontLink().equals("404")) {
                gameInfo.setCoverImage(game.getGameID(), childview, game.getFrontLink(), pos);
            } else {
                ImageView preview = (ImageView) childview.findViewById(R.id.game_icon);
                preview.setImageResource(R.drawable.boxart);
            }
        } else if (VirtualMachineManager.IsLoadableExecutableFileName(game.getFile().getName())) {
            ImageView preview = (ImageView) childview.findViewById(R.id.game_icon);
            preview.setImageResource(R.drawable.boxart);
            childview.findViewById(R.id.childview).setOnLongClickListener(null);
        } else {
            childview.findViewById(R.id.childview).setOnLongClickListener(null);
            // passing game, as to pass and use (if) any user defined values
            final GameInfoStruct gameStats = gameInfo.getGameInfo(game.getFile(), childview, game, pos);

            if (gameStats != null) {
                games.set(pos, gameStats);
                childview.findViewById(R.id.childview).setOnLongClickListener(
                        gameInfo.configureLongClick(gameStats));

                if (gameStats.getFrontLink() != null && !gameStats.getFrontLink().equals("404")) {
                    gameInfo.setCoverImage(gameStats.getGameID(), childview, gameStats.getFrontLink(), pos);
                }
            } else {
                ImageView preview = (ImageView) childview.findViewById(R.id.game_icon);
                preview.setImageResource(R.drawable.boxart);
            }
        }



        childview.findViewById(R.id.childview).setOnClickListener(new View.OnClickListener() {
            public void onClick(View view) {
                launchGame(game, getContext());
                return;
            }
        });
        return childview;

    }

}
