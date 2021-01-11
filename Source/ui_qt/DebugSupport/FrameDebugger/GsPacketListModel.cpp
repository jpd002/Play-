#include "GsPacketListModel.h"
#include "GsPacketData.h"

#include "string_format.h"

#include <QStringList>

PacketTreeModel::PacketTreeModel(QWidget* parent)
    : QAbstractItemModel(parent)
    , m_rootItem(new GsPacketData("Packets", 0))
{
}

PacketTreeModel::~PacketTreeModel()
{
	delete m_rootItem;
}

int PacketTreeModel::columnCount(const QModelIndex& parent) const
{
	return 1;
}

QVariant PacketTreeModel::data(const QModelIndex& index, int role) const
{
	if(!index.isValid())
		return QVariant();

	if(role == Qt::FontRole)
	{
		GsPacketData* item = static_cast<GsPacketData*>(index.internalPointer());
		if(item->IsDrawKick())
		{
			QFont boldFont;
			boldFont.setBold(true);
			return boldFont;
		}
	}
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
		return m_rootItem->data(section);

	return QVariant();
}

QModelIndex PacketTreeModel::index(int row, int column, const QModelIndex& parent) const
{
	if(!hasIndex(row, column, parent))
		return QModelIndex();

	GsPacketData* parentItem;

	if(!parent.isValid())
		parentItem = m_rootItem;
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

	if(parentItem == m_rootItem)
		return QModelIndex();

	return createIndex(parentItem->row(), 0, parentItem);
}

int PacketTreeModel::rowCount(const QModelIndex& parent) const
{
	GsPacketData* parentItem;
	if(parent.column() > 0)
		return 0;

	if(!parent.isValid())
		parentItem = m_rootItem;
	else
		parentItem = static_cast<GsPacketData*>(parent.internalPointer());

	return parentItem->childCount();
}

void PacketTreeModel::setupModelData(CFrameDump& m_frameDump)
{
	uint32 packetIndex = 0, cmdIndex = 0;
	m_drawKickIndexInfo.clear();
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

		auto parent = new GsPacketData(packetDescription.c_str(), cmdIndex, m_rootItem);
		m_rootItem->appendChild(parent);

		QVector<QVariant> columnData;
		columnData.reserve(packet.registerWrites.size());
		auto cmdIndexStart = cmdIndex;
		for(const auto& registerWrite : packet.registerWrites)
		{
			bool isKickDraw = drawingKicks.count(cmdIndex) > 0;
			auto packetWriteDescription = CGSHandler::DisassembleWrite(registerWrite.first, registerWrite.second);
			auto GsPacketDataText = string_format("%04X: %s", cmdIndex - cmdIndexStart, packetWriteDescription.c_str());
			parent->appendChild(new GsPacketData(GsPacketDataText.c_str(), cmdIndex, parent, isKickDraw));
			if(isKickDraw)
			{
				m_drawKickIndexInfo.push_back({packetIndex, cmdIndex - cmdIndexStart, cmdIndex});
			}
			++cmdIndex;
		}
		++packetIndex;
	}
}

const std::vector<PacketTreeModel::DrawKickIndexInfo>& PacketTreeModel::GetDrawKickIndexes()
{
	return m_drawKickIndexInfo;
}
