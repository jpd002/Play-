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

	CQtMemoryViewModel(QObject*);
	~CQtMemoryViewModel() = default;

	int rowCount(const QModelIndex& parent = QModelIndex()) const Q_DECL_OVERRIDE;
	int columnCount(const QModelIndex& parent = QModelIndex()) const Q_DECL_OVERRIDE;
	QVariant data(const QModelIndex& index, int role = Qt::DisplayRole) const Q_DECL_OVERRIDE;

	void Redraw();

	unsigned int GetBytesPerRow() const;
	void SetBytesPerRow(unsigned int);

	void SetActiveUnit(int);
	int GetActiveUnit() const;
	unsigned int GetBytesPerUnit() const;
	unsigned int CharsPerUnit() const;

	void SetData(getByteProto, int);

	uint32 TranslateModelIndexToAddress(const QModelIndex&) const;
	QModelIndex TranslateAddressToModelIndex(uint32) const;

	static std::vector<UNITINFO> g_units;

protected:
	QVariant headerData(int section, Qt::Orientation orientation, int role) const Q_DECL_OVERRIDE;

private:
	uint8 GetByte(uint32) const;
	std::string RenderByteUnit(uint32) const;
	std::string RenderWordUnit(uint32) const;
	std::string RenderSingleUnit(uint32) const;

	getByteProto m_getByte;
	int m_activeUnit = 0;
	unsigned int m_size = 0;
	unsigned int m_bytesPerRow = 0x2;
};
