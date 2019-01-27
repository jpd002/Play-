package com.virtualapplications.play;

import android.content.Context;
import android.support.v7.app.AppCompatActivity;
import android.view.LayoutInflater;
import android.view.View;
import android.view.ViewGroup;
import android.widget.ArrayAdapter;
import android.widget.ImageView;
import android.widget.TextView;
import android.widget.Toast;

import com.virtualapplications.play.database.GameInfo;

import java.lang.ref.WeakReference;
import java.util.List;

public class GamesAdapter extends ArrayAdapter<Bootable>
{
	private final int layoutid;
	private List<Bootable> games;
	private GameInfo gameInfo;
	private final WeakReference<AppCompatActivity> _activity;

	private final View.OnLongClickListener no_long_click = new View.OnLongClickListener()

	{
		@Override
		public boolean onLongClick (View view)
		{
			Toast.makeText(GamesAdapter.this._activity.get(), "No data to display.", Toast.LENGTH_SHORT).show();
			return true;
		}
	};
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

		View.OnLongClickListener long_click;
		if(!game.title.isEmpty() || !game.overview.isEmpty())
		{
			long_click = gameInfo.configureLongClick(game);
		}
		else
		{
			long_click = no_long_click;
		}
		viewHolder.childview.setOnLongClickListener(long_click);

		if(!game.coverUrl.isEmpty())
		{
			gameInfo.setCoverImage(game.discId, viewHolder, game.coverUrl, pos);
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
