#include "playlistmodel.h"
#include "PsfTags.h"

PlaylistModel::PlaylistModel(QObject *parent)
	:QAbstractTableModel(parent)
{
}

PlaylistModel::~PlaylistModel()
{
	m_playlist.clear();
}

int PlaylistModel::addPlaylistItem(std::string fileName, CPsfBase::TagMap tagsmap)
{
	emit QAbstractTableModel::beginInsertRows(QModelIndex(), rowCount(), rowCount());

	auto tags = CPsfTags(tagsmap);

	std::wstring game = tags.GetTagValue("game");
	std::wstring title = tags.GetTagValue("title");
	std::wstring length = tags.GetTagValue("length");
	Playlist::Item item(game, title, length, fileName);
	m_playlist.push_back(item);

	emit QAbstractTableModel::endInsertRows();

	return m_playlist.size()-1;
}

int PlaylistModel::removePlaylistItem(int index)
{
	emit QAbstractTableModel::beginRemoveRows(QModelIndex(), index, index);
	m_playlist.erase(m_playlist.begin()+index);
	emit QAbstractTableModel::endRemoveRows();

	return m_playlist.size()-1;
}

Playlist::Item* PlaylistModel::at(int row)
{
	return &m_playlist.at(row);
}

int PlaylistModel::rowCount(const QModelIndex & /*parent*/) const
{
	return m_playlist.size();
}

int PlaylistModel::columnCount(const QModelIndex & /*parent*/) const
{
	return 3;
}

QVariant PlaylistModel::data(const QModelIndex &index, int role) const
{
	if(role == Qt::DisplayRole)
	{
		auto item = m_playlist.at(index.row());
		const wchar_t* val;
		switch(index.column())
		{
		case 0:
			val = item.game.c_str();
			break;
		case 1:
			val = item.title.c_str();
			break;
		case 2:
			val = item.length.c_str();
			break;
		}
		return QVariant(QString::fromWCharArray(val));
	}
	return QVariant();
}

bool PlaylistModel::setHeaderData(int section, Qt::Orientation orientation, const QVariant &value, int role)
{
	if(orientation == Qt::Horizontal)
	{

		if(role == Qt::DisplayRole)
		{
			m_header.insert(section, value);
			return true;
		}
	}
	return QAbstractTableModel::setHeaderData(section, orientation, value, role);
}

QVariant PlaylistModel::headerData(int section, Qt::Orientation orientation, int role) const
{
	if(orientation == Qt::Horizontal)
	{
	   if(role == Qt::DisplayRole)
		{
			if(section < m_header.size())
				return m_header.at(section);
		}
	}

	return QAbstractTableModel::headerData(section, orientation, role);
}
