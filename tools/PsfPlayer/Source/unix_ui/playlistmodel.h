#pragma once

#include "PlaylistItem.h"
#include "PsfBase.h"
#include <QAbstractTableModel>
#include <vector>

class PlaylistModel : public QAbstractTableModel
{
	Q_OBJECT
public:
	PlaylistModel(QObject* parent);
	~PlaylistModel();

	bool setHeaderData(int section, Qt::Orientation orientation, const QVariant& value, int role = Qt::EditRole) Q_DECL_OVERRIDE;
	int rowCount(const QModelIndex& parent = QModelIndex()) const Q_DECL_OVERRIDE;

	int             addPlaylistItem(std::string, CPsfBase::TagMap);
	int             removePlaylistItem(int);
	Playlist::Item* at(int);

protected:
	QVariant headerData(int section, Qt::Orientation orientation, int role) const Q_DECL_OVERRIDE;

private:
	int columnCount(const QModelIndex& parent = QModelIndex()) const Q_DECL_OVERRIDE;
	QVariant data(const QModelIndex& index, int role = Qt::DisplayRole) const Q_DECL_OVERRIDE;

	std::vector<Playlist::Item> m_playlist;
	QVariantList                m_header;
};
