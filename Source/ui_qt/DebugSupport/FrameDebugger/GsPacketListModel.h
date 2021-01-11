#pragma once

#include <QAbstractItemModel>
#include <QModelIndex>
#include <QVariant>
#include <QWidget>
#include "FrameDump.h"
#include <QMetaObject>

class GsPacketData;

class PacketTreeModel : public QAbstractItemModel
{
	Q_OBJECT

public:
	explicit PacketTreeModel(QWidget* = nullptr);
	~PacketTreeModel();

	struct DrawKickIndexInfo
	{
		uint32 parentIndex;
		uint32 childIndex;
		uint32 cmdIndex;
	};

	QVariant data(const QModelIndex&, int) const override;
	Qt::ItemFlags flags(const QModelIndex&) const override;
	QVariant headerData(int, Qt::Orientation, int = Qt::DisplayRole) const override;
	QModelIndex index(int, int, const QModelIndex& = QModelIndex()) const override;
	QModelIndex parent(const QModelIndex&) const override;
	int rowCount(const QModelIndex& = QModelIndex()) const override;
	int columnCount(const QModelIndex& = QModelIndex()) const override;

	void setupModelData(CFrameDump&);
	const std::vector<DrawKickIndexInfo>& GetDrawKickIndexes();

private:
	GsPacketData* m_rootItem;
	std::vector<DrawKickIndexInfo> m_drawKickIndexInfo;
};
