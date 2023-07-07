#pragma once

#include <QAbstractTableModel>
#include <vector>
#include "Playlist.h"
#include "PlaylistDiscoveryService.h"

class PlaylistModel : public QAbstractTableModel
{
	Q_OBJECT
public:
	PlaylistModel(QObject* parent);
	virtual ~PlaylistModel() = default;

	bool setHeaderData(int section, Qt::Orientation orientation, const QVariant& value, int role = Qt::EditRole) Q_DECL_OVERRIDE;
	int rowCount(const QModelIndex& parent = QModelIndex()) const Q_DECL_OVERRIDE;

	void addItem(fs::path);
	void addArchive(fs::path);
	int removeItem(int);
	void clearPlaylist();

	const CPlaylist::ITEM* at(int);
	fs::path getArchivePath(int);

public slots:
	void updatePlaylist();

protected:
	QVariant headerData(int section, Qt::Orientation orientation, int role) const Q_DECL_OVERRIDE;

private:
	int columnCount(const QModelIndex& parent = QModelIndex()) const Q_DECL_OVERRIDE;
	QVariant data(const QModelIndex& index, int role = Qt::DisplayRole) const Q_DECL_OVERRIDE;

private:
	QTimer* m_timer = nullptr;
	CPlaylist m_playlist;
	CPlaylistDiscoveryService m_playlistDiscoveryService;
	QVariantList m_header;
};
