#include "playlistmodel.h"
#include <QTimer>
#include "PsfTags.h"
#include "PsfLoader.h"
#include "TimeToString.h"

PlaylistModel::PlaylistModel(QObject* parent)
    : QAbstractTableModel(parent)
{
	m_timer = new QTimer(this);
	connect(m_timer, SIGNAL(timeout()), this, SLOT(updatePlaylist()));
	m_timer->start(250);
}

void PlaylistModel::updatePlaylist()
{
	auto onItemUpdateConnection = m_playlist.OnItemUpdate.Connect(
	    [this](unsigned int itemIndex, const CPlaylist::ITEM& item) {
		    emit QAbstractTableModel::dataChanged(createIndex(itemIndex, 0), createIndex(itemIndex, m_header.size() - 1));
	    });
	m_playlistDiscoveryService.ProcessPendingItems(m_playlist);
	onItemUpdateConnection.reset();
}

void PlaylistModel::addItem(fs::path itemPath)
{
	emit QAbstractTableModel::beginInsertRows(QModelIndex(), rowCount(), rowCount());

	CPlaylist::ITEM item;
	item.path = itemPath.wstring();
	unsigned int itemId = m_playlist.InsertItem(item);
	m_playlistDiscoveryService.AddItemInRun(item.path, std::wstring(), itemId);

	emit QAbstractTableModel::endInsertRows();
}

void PlaylistModel::addArchive(fs::path archivePath)
{
	auto archive(CPsfArchive::CreateFromPath(archivePath));
	unsigned int archiveId = m_playlist.InsertArchive(archivePath.wstring());

	std::vector<CPlaylist::ITEM> newItems;

	for(const auto& fileInfo : archive->GetFiles())
	{
		auto pathToken = CArchivePsfStreamProvider::GetPathTokenFromFilePath(fileInfo.name);
		auto fileExtension = pathToken.GetExtension();
		if(CPlaylist::IsLoadableExtension(fileExtension))
		{
			CPlaylist::ITEM newItem;
			newItem.path = pathToken.GetWidePath();
			newItem.title = newItem.path;
			newItem.length = 0;
			newItem.archiveId = archiveId;
			newItems.emplace_back(std::move(newItem));
		}
	}

	emit QAbstractTableModel::beginInsertRows(QModelIndex(), rowCount(), rowCount() + newItems.size() - 1);
	for(const auto& newItem : newItems)
	{
		unsigned int itemId = m_playlist.InsertItem(newItem);
		m_playlistDiscoveryService.AddItemInRun(newItem.path, archivePath, itemId);
	}
	emit QAbstractTableModel::endInsertRows();
}

int PlaylistModel::removeItem(int index)
{
	emit QAbstractTableModel::beginRemoveRows(QModelIndex(), index, index);
	m_playlist.DeleteItem(index);
	emit QAbstractTableModel::endRemoveRows();

	return m_playlist.GetItemCount() - 1;
}

void PlaylistModel::clearPlaylist()
{
	m_playlistDiscoveryService.ResetRun();
	if(m_playlist.GetItemCount() == 0)
	{
		return;
	}
	emit QAbstractTableModel::beginRemoveRows(QModelIndex(), 0, m_playlist.GetItemCount() - 1);
	m_playlist.Clear();
	emit QAbstractTableModel::endRemoveRows();
}

const CPlaylist::ITEM* PlaylistModel::at(int row)
{
	return &m_playlist.GetItem(row);
}

fs::path PlaylistModel::getArchivePath(int archiveId)
{
	if(archiveId == 0)
	{
		return fs::path();
	}
	return m_playlist.GetArchive(archiveId);
}

int PlaylistModel::rowCount(const QModelIndex& /*parent*/) const
{
	return m_playlist.GetItemCount();
}

int PlaylistModel::columnCount(const QModelIndex& /*parent*/) const
{
	return m_header.size();
}

QVariant PlaylistModel::data(const QModelIndex& index, int role) const
{
	if(role == Qt::DisplayRole)
	{
		const auto& item = m_playlist.GetItem(index.row());
		std::wstring val;
		switch(index.column())
		{
		case 0:
			val = item.title;
			break;
		case 1:
			val = TimeToString<std::wstring>(item.length);
			break;
		}
		return QVariant(QString::fromStdWString(val));
	}
	return QVariant();
}

bool PlaylistModel::setHeaderData(int section, Qt::Orientation orientation, const QVariant& value, int role)
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
