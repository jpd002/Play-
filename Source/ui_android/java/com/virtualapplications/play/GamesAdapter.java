package com.virtualapplications.play;

import android.content.Context;
import android.support.v7.app.AppCompatActivity;
import android.view.LayoutInflater;
import android.view.View;
import android.view.ViewGroup;
import android.widget.ArrayAdapter;
import android.widget.ImageView;
import android.widget.TextView;

import com.virtualapplications.play.database.GameInfo;

import java.lang.ref.WeakReference;
import java.util.List;

public class GamesAdapter extends ArrayAdapter<Bootable>
{
	private final int layoutid;
	private List<Bootable> games;
	private GameInfo gameInfo;
	private final WeakReference<AppCompatActivity> _activity;

	public static class CoverViewHolder
	{
		CoverViewHolder(View v)
		{
			this.childview = v;
			this.gameImageView = (ImageView)v.findViewById(R.id.game_icon);
			this.gameTextView = (TextView)v.findViewById(R.id.game_text);
			this.currentPositionView = (TextView)v.findViewById(R.id.currentPosition);
		}

		public View childview;
		public ImageView gameImageView;
		public TextView gameTextView;
		public TextView currentPositionView;
	}

	public GamesAdapter(AppCompatActivity activity, int ResourceId, List<Bootable> images, GameInfo gameInfo)
	{
		super(activity, ResourceId, images);
		this.games = images;
		this.layoutid = ResourceId;
		this.gameInfo = gameInfo;
		this._activity = new WeakReference<AppCompatActivity>(activity);
	}

	public int getCount()
	{
		return games.size();
	}

	public Bootable getItem(int position)
	{
		return games.get(position);
	}

	public long getItemId(int position)
	{
		return position;
	}

	@Override
	public View getView(int position, View v, ViewGroup parent)
	{
		CoverViewHolder viewHolder;
		if(v == null)
		{
			LayoutInflater vi = (LayoutInflater)getContext().getSystemService(Context.LAYOUT_INFLATER_SERVICE);
			v = vi.inflate(layoutid, null);
			viewHolder = new CoverViewHolder(v.findViewById(R.id.childview));
			v.setTag(viewHolder);
		}
		else
		{
			viewHolder = (CoverViewHolder)v.getTag();
			viewHolder.gameTextView.setText("");
			viewHolder.gameTextView.setVisibility(View.VISIBLE);
			viewHolder.gameImageView.setImageResource(R.drawable.boxart);
		}

		viewHolder.currentPositionView.setText(String.valueOf(position));

		final Bootable game = games.get(position);
		if(game != null)
		{
			createListItem(game, viewHolder, position);
		}
		return v;
	}

	private void createListItem(final Bootable game, final CoverViewHolder viewHolder, int pos)
	{
		viewHolder.gameTextView.setText(game.title);
		//If user has set values, then read them, if not read from database
		if(game.coverUrl != null && !game.coverUrl.equals(""))
		{

			viewHolder.childview.setOnLongClickListener(gameInfo.configureLongClick(game));
			if(!game.coverUrl.equals("404"))
			{
				gameInfo.setCoverImage(game.discId, viewHolder, game.coverUrl, pos);
			}
			else
			{
				viewHolder.gameImageView.setImageResource(R.drawable.boxart);
			}
		}
		else if(VirtualMachineManager.IsLoadableExecutableFileName(game.path))
		{
			viewHolder.gameImageView.setImageResource(R.drawable.boxart);
			viewHolder.childview.setOnLongClickListener(null);
		}
		else
		{
			viewHolder.childview.setOnLongClickListener(null);
			viewHolder.gameImageView.setImageResource(R.drawable.boxart);
		}

		viewHolder.childview.setOnClickListener(new View.OnClickListener()
		{
			public void onClick(View view)
			{
				if(_activity.get() != null)
				{
					if(_activity.get() instanceof MainActivity)
					{
						((MainActivity)_activity.get()).launchGame(game);
					}
				}
				return;
			}
		});
	}
}
