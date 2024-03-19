#include "QtMemoryViewModel.h"
#include <cmath>
#include "string_format.h"

// clang-format off
std::vector<CQtMemoryViewModel::UNITINFO> CQtMemoryViewModel::g_units =
{
	{1, 2, &CQtMemoryViewModel::RenderByteUnit, "8-bit Integers"},
	{4, 8, &CQtMemoryViewModel::RenderWordUnit, "32-bit Integers"},
	{4, 11, &CQtMemoryViewModel::RenderSingleUnit, "Single Precision Floating Point Numbers"}
};
// clang-format on

CQtMemoryViewModel::CQtMemoryViewModel(QObject* parent)
    : QAbstractTableModel(parent)
{
}

int CQtMemoryViewModel::rowCount(const QModelIndex& /*parent*/) const
{
	return std::ceil((m_size * 1.f) / m_bytesPerRow);
}

int CQtMemoryViewModel::columnCount(const QModelIndex& /*parent*/) const
{
	assert(m_bytesPerRow % GetBytesPerUnit() == 0);
	return (m_bytesPerRow / GetBytesPerUnit()) + 1;
}

QVariant CQtMemoryViewModel::data(const QModelIndex& index, int role) const
{
	if(role == Qt::DisplayRole)
	{
		if(index.column() < columnCount() - 1)
		{
			uint32 address = TranslateModelIndexToAddress(index);
			return (this->*(g_units[m_activeUnit].renderer))(address).c_str();
		}
		else
		{
			uint32 address = TranslateModelIndexToAddress(this->index(index.row(), 0));
			std::string res;
			for(auto j = 0; j < m_bytesPerRow; j++)
			{
				if((address + j) >= m_size)
				{
					res += " ";
				}
				else
				{
					uint8 value = GetByte(address + j);
					if(value < 0x20 || value > 0x7F)
					{
						value = '.';
					}
					char valueString[2];
					valueString[0] = value;
					valueString[1] = 0x00;
					res += valueString;
				}
			}
			return res.c_str();
		}
		return QVariant();
	}
	return QVariant();
}

QVariant CQtMemoryViewModel::headerData(int section, Qt::Orientation orientation, int role) const
{
	if(role == Qt::DisplayRole)
	{
		if(orientation == Qt::Horizontal)
		{
			return "";
		}
		else
		{
			auto address = section * m_bytesPerRow;
			return QString::fromStdString(string_format("0x%08X", address));
		}
	}

	return QAbstractTableModel::headerData(section, orientation, role);
}

void CQtMemoryViewModel::Redraw()
{
	emit QAbstractTableModel::beginResetModel();
	emit QAbstractTableModel::endResetModel();
}

uint8 CQtMemoryViewModel::GetByte(uint32 nAddress) const
{
	if(!m_getByte)
		return 0;

	return m_getByte(nAddress);
}

std::string CQtMemoryViewModel::RenderByteUnit(uint32 address) const
{
	if(address >= m_size)
	{
		return "--";
	}
	else
	{
		uint8 unitValue = GetByte(address);
		return string_format("%02X", unitValue);
	}
}

std::string CQtMemoryViewModel::RenderWordUnit(uint32 address) const
{
	if(address >= m_size)
	{
		return "--------";
	}
	else
	{
		uint32 unitValue =
		    (GetByte(address + 0) << 0) |
		    (GetByte(address + 1) << 8) |
		    (GetByte(address + 2) << 16) |
		    (GetByte(address + 3) << 24);
		return string_format("%08X", unitValue);
	}
}

std::string CQtMemoryViewModel::RenderSingleUnit(uint32 address) const
{
	if(address >= m_size)
	{
		return "-----------";
	}
	else
	{
		uint32 unitValue =
		    (GetByte(address + 0) << 0) |
		    (GetByte(address + 1) << 8) |
		    (GetByte(address + 2) << 16) |
		    (GetByte(address + 3) << 24);
		return string_format("%+04.4e", *reinterpret_cast<const float*>(&unitValue));
	}
}

unsigned int CQtMemoryViewModel::GetBytesPerRow() const
{
	return m_bytesPerRow;
}

void CQtMemoryViewModel::SetBytesPerRow(unsigned int bytesPerRow)
{
	m_bytesPerRow = bytesPerRow;
}

uint32 CQtMemoryViewModel::TranslateModelIndexToAddress(const QModelIndex& index) const
{
	uint32 address = index.row() * m_bytesPerRow;
	if((columnCount() - 1) != index.column())
	{
		address += index.column() * GetBytesPerUnit();
	}
	return address;
}

QModelIndex CQtMemoryViewModel::TranslateAddressToModelIndex(uint32 address) const
{
	if(static_cast<int32>(address) < 0)
	{
		return index(0, 0);
	}
	if(static_cast<int32>(address) >= m_size)
	{
		address = (m_size - 1);
	}
	uint32 row = address / m_bytesPerRow;
	uint32 col = address - (row * m_bytesPerRow);
	col /= GetBytesPerUnit();
	return index(row, col);
}

void CQtMemoryViewModel::SetActiveUnit(int index)
{
	m_activeUnit = index;
}

int CQtMemoryViewModel::GetActiveUnit() const
{
	return m_activeUnit;
}

unsigned int CQtMemoryViewModel::GetBytesPerUnit() const
{
	return g_units[m_activeUnit].bytesPerUnit;
}

unsigned int CQtMemoryViewModel::CharsPerUnit() const
{
	return g_units[m_activeUnit].charsPerUnit;
}

void CQtMemoryViewModel::SetData(getByteProto getByte, int size)
{
	m_getByte = getByte;
	m_size = size;
	Redraw();
}
