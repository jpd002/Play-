#include "GsPacketListModel.h"
#include "GsPacketData.h"

#include "string_format.h"

#include <QStringList>

PacketTreeModel::PacketTreeModel(QWidget* parent)
    : QAbstractItemModel(parent)
    , rootItem(new GsPacketData("Packets", 0))
{
}

PacketTreeModel::~PacketTreeModel()
{
	delete rootItem;
}

int PacketTreeModel::columnCount(const QModelIndex& parent) const
{
	return 1;
}

QVariant PacketTreeModel::data(const QModelIndex& index, int role) const
{
	if(!index.isValid())
		return QVariant();

	if(role != Qt::DisplayRole)
		return QVariant();

	GsPacketData* item = static_cast<GsPacketData*>(index.internalPointer());

	return item->data(index.column());
}

Qt::ItemFlags PacketTreeModel::flags(const QModelIndex& index) const
{
	if(!index.isValid())
		return Qt::NoItemFlags;

	return QAbstractItemModel::flags(index);
}

QVariant PacketTreeModel::headerData(int section, Qt::Orientation orientation, int role) const
{
	if(orientation == Qt::Horizontal && role == Qt::DisplayRole)
		return rootItem->data(section);

	return QVariant();
}

QModelIndex PacketTreeModel::index(int row, int column, const QModelIndex& parent) const
{
	if(!hasIndex(row, column, parent))
		return QModelIndex();

	GsPacketData* parentItem;

	if(!parent.isValid())
		parentItem = rootItem;
	else
		parentItem = static_cast<GsPacketData*>(parent.internalPointer());

	GsPacketData* childItem = parentItem->child(row);
	if(childItem)
		return createIndex(row, column, childItem);
	return QModelIndex();
}

QModelIndex PacketTreeModel::parent(const QModelIndex& index) const
{
	if(!index.isValid())
		return QModelIndex();

	GsPacketData* childItem = static_cast<GsPacketData*>(index.internalPointer());
	GsPacketData* parentItem = childItem->parent();

	if(parentItem == rootItem)
		return QModelIndex();

	return createIndex(parentItem->row(), 0, parentItem);
}

int PacketTreeModel::rowCount(const QModelIndex& parent) const
{
	GsPacketData* parentItem;
	if(parent.column() > 0)
		return 0;

	if(!parent.isValid())
		parentItem = rootItem;
	else
		parentItem = static_cast<GsPacketData*>(parent.internalPointer());

	return parentItem->childCount();
}

void PacketTreeModel::setupModelData(CFrameDump& m_frameDump)
{
	uint32 packetIndex = 0, cmdIndex = 0;
	const auto& drawingKicks = m_frameDump.GetDrawingKicks();
	for(const auto& packet : m_frameDump.GetPackets())
	{
		bool isRegisterPacket = packet.imageData.empty();

		auto lowerBoundIterator = drawingKicks.upper_bound(cmdIndex);
		auto upperBoundIterator = drawingKicks.lower_bound(cmdIndex + packet.registerWrites.size());

		int kickCount = static_cast<int>(std::distance(lowerBoundIterator, upperBoundIterator));

		std::string packetDescription;
		if(isRegisterPacket)
		{
			packetDescription = string_format("%d: Register Packet (Write Count: %d, Draw Count: %d, Path: %d)",
			                                  packetIndex, packet.registerWrites.size(), kickCount, packet.metadata.pathIndex);
		}
		else
		{
			packetDescription = string_format("%d: Image Packet (Size: 0x%08X)", packetIndex, packet.imageData.size());
		}

		auto parent = new GsPacketData(packetDescription.c_str(), cmdIndex, rootItem);
		rootItem->appendChild(parent);

		QVector<QVariant> columnData;
		columnData.reserve(packet.registerWrites.size());
		auto cmdIndexStart = cmdIndex;
		for(const auto& registerWrite : packet.registerWrites)
		{
			auto packetWriteDescription = CGSHandler::DisassembleWrite(registerWrite.first, registerWrite.second);
			auto GsPacketDataText = string_format("%04X: %s", cmdIndex - cmdIndexStart, packetWriteDescription.c_str());
			parent->appendChild(new GsPacketData(GsPacketDataText.c_str(), cmdIndex, parent));
			++cmdIndex;
		}
		++packetIndex;
	}
}
