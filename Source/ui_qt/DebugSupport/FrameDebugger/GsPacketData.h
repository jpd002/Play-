#pragma once

#include <QVariant>
#include <QVector>

class GsPacketData
{
public:
	explicit GsPacketData(const QVariant&, int, GsPacketData* = nullptr, bool = false);
	~GsPacketData();

	void appendChild(GsPacketData*);

	GsPacketData* child(int row);
	QVector<GsPacketData*> Children();
	int childCount() const;
	QVariant data(int column) const;
	int row() const;
	GsPacketData* parent();

	int GetCmdIndex() const;
	bool IsDrawKick();

private:
	QVector<GsPacketData*> m_children;
	QVariant m_data;
	GsPacketData* m_parent;
	int m_cmdIndex = 0;
	bool m_isDrawKick = false;
};
