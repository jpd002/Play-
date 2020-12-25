#pragma once

#include <QAbstractTableModel>
#include <functional>

#include "Types.h"

class CQtMemoryViewModel : public QAbstractTableModel
{
	Q_OBJECT
public:
	typedef std::function<uint8(uint32)> getByteProto;

	typedef std::string (CQtMemoryViewModel::*UnitRenderer)(uint32) const;
	struct UNITINFO
	{
		unsigned int bytesPerUnit = 0;
		unsigned int charsPerUnit = 0;
		UnitRenderer renderer = nullptr;
		const char* description = nullptr;
	};

	CQtMemoryViewModel(QObject*, getByteProto = nullptr, int = 0);
	~CQtMemoryViewModel() = default;

	int rowCount(const QModelIndex& parent = QModelIndex()) const Q_DECL_OVERRIDE;
	int columnCount(const QModelIndex& parent = QModelIndex()) const Q_DECL_OVERRIDE;
	QVariant data(const QModelIndex& index, int role = Qt::DisplayRole) const Q_DECL_OVERRIDE;
	void DoubleClicked(const QModelIndex& index, QWidget* parent = nullptr);

	void Redraw();

	unsigned int BytesForCurrentLine() const;
	void SetColumnCount(unsigned int);
	unsigned int CharsPerUnit() const;

	void SetActiveUnit(int);
	int GetActiveUnit();
	unsigned int GetBytesPerUnit() const;

	void SetData(getByteProto, int);

	static std::vector<UNITINFO> g_units;

protected:
	QVariant headerData(int section, Qt::Orientation orientation, int role) const Q_DECL_OVERRIDE;

private:
	uint8 GetByte(uint32) const;
	std::string RenderByteUnit(uint32) const;
	std::string RenderWordUnit(uint32) const;
	std::string RenderSingleUnit(uint32) const;

	getByteProto m_getByte = nullptr;
	int m_activeUnit;
	unsigned int m_size;
	std::atomic<unsigned int> m_columnCount = 0x2;
};
