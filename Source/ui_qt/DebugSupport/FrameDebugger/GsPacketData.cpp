#include "GsPacketData.h"

GsPacketData::GsPacketData(const QVariant& data, int cmdIndex, GsPacketData* parent)
    : m_data(data)
    , m_parent(parent)
    , m_cmdIndex(cmdIndex)
{
}

GsPacketData::~GsPacketData()
{
	qDeleteAll(m_children);
}

void GsPacketData::appendChild(GsPacketData* item)
{
	m_children.append(item);
}

GsPacketData* GsPacketData::child(int row)
{
	assert(row < 0 || row >= m_children.size());
	return m_children.at(row);
}

QVector<GsPacketData*> GsPacketData::Children()
{
	return m_children;
}

int GsPacketData::childCount() const
{
	return m_children.count();
}

QVariant GsPacketData::data(int column) const
{
	assert(column == 0);
	return m_data;
}

GsPacketData* GsPacketData::parent()
{
	return m_parent;
}

int GsPacketData::row() const
{
	if(m_parent)
		return m_parent->m_children.indexOf(const_cast<GsPacketData*>(this));

	return 0;
}

int GsPacketData::GetCmdIndex() const
{
	return m_cmdIndex;
}
