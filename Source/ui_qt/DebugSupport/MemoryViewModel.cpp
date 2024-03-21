#include "MemoryViewModel.h"
#include <cmath>
#include "string_format.h"

// clang-format off
const std::vector<CMemoryViewModel::UNITINFO> CMemoryViewModel::g_units =
{
	{1, 2, &CMemoryViewModel::RenderByteUnit, "8-bit Integers"},
	{4, 8, &CMemoryViewModel::RenderWordUnit, "32-bit Integers"},
	{4, 11, &CMemoryViewModel::RenderSingleUnit, "Single Precision Floating Point Numbers"}
};
// clang-format on

CMemoryViewModel::CMemoryViewModel(QObject* parent)
    : QAbstractTableModel(parent)
{
}

int CMemoryViewModel::rowCount(const QModelIndex& /*parent*/) const
{
	return std::ceil((m_windowSize * 1.f) / m_bytesPerRow);
}

int CMemoryViewModel::columnCount(const QModelIndex& /*parent*/) const
{
	assert(m_bytesPerRow % GetBytesPerUnit() == 0);
	return (m_bytesPerRow / GetBytesPerUnit()) + 1;
}

QVariant CMemoryViewModel::data(const QModelIndex& index, int role) const
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

QVariant CMemoryViewModel::headerData(int section, Qt::Orientation orientation, int role) const
{
	if(role == Qt::DisplayRole)
	{
		if(orientation == Qt::Horizontal)
		{
			return "";
		}
		else
		{
			auto address = (section * m_bytesPerRow) + m_windowStart;
			return QString::fromStdString(string_format("0x%08X", address));
		}
	}

	return QAbstractTableModel::headerData(section, orientation, role);
}

void CMemoryViewModel::Redraw()
{
	emit QAbstractTableModel::beginResetModel();
	emit QAbstractTableModel::endResetModel();
}

uint8 CMemoryViewModel::GetByte(uint32 nAddress) const
{
	if(!m_getByte)
		return 0;

	return m_getByte(nAddress);
}

std::string CMemoryViewModel::RenderByteUnit(uint32 address) const
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

std::string CMemoryViewModel::RenderWordUnit(uint32 address) const
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

std::string CMemoryViewModel::RenderSingleUnit(uint32 address) const
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

unsigned int CMemoryViewModel::GetBytesPerRow() const
{
	return m_bytesPerRow;
}

void CMemoryViewModel::SetBytesPerRow(unsigned int bytesPerRow)
{
	m_bytesPerRow = bytesPerRow;
}

void CMemoryViewModel::SetWindowCenter(uint32 windowCenter)
{
	int64 lowerBound = static_cast<int64>(windowCenter) - static_cast<int64>(m_windowSize / 2);
	int64 upperBound = static_cast<int64>(windowCenter) + static_cast<int64>(m_windowSize / 2);
	if(lowerBound < 0)
	{
		m_windowStart = 0;
	}
	else if(upperBound >= m_size)
	{
		m_windowStart = static_cast<uint32>(m_size - static_cast<uint64>(m_windowSize));
	}
	else
	{
		m_windowStart = static_cast<uint32>(lowerBound);
	}
}

uint32 CMemoryViewModel::TranslateModelIndexToAddress(const QModelIndex& index) const
{
	uint32 address = index.row() * m_bytesPerRow;
	if((columnCount() - 1) != index.column())
	{
		address += index.column() * GetBytesPerUnit();
	}
	return address + m_windowStart;
}

QModelIndex CMemoryViewModel::TranslateAddressToModelIndex(uint32 address) const
{
	if(address <= m_windowStart)
	{
		return index(0, 0);
	}
	address -= m_windowStart;
	if(address >= m_windowSize)
	{
		address = (m_windowSize - 1);
	}
	uint32 row = address / m_bytesPerRow;
	uint32 col = address - (row * m_bytesPerRow);
	col /= GetBytesPerUnit();
	return index(row, col);
}

void CMemoryViewModel::SetActiveUnit(int index)
{
	m_activeUnit = index;
}

int CMemoryViewModel::GetActiveUnit() const
{
	return m_activeUnit;
}

unsigned int CMemoryViewModel::GetBytesPerUnit() const
{
	return g_units[m_activeUnit].bytesPerUnit;
}

unsigned int CMemoryViewModel::CharsPerUnit() const
{
	return g_units[m_activeUnit].charsPerUnit;
}

void CMemoryViewModel::SetData(getByteProto getByte, uint64 size, uint32 windowSize)
{
	if(windowSize == 0)
	{
		assert(size <= UINT32_MAX);
		windowSize = static_cast<uint32>(size);
	}
	m_getByte = getByte;
	m_size = size;
	m_windowStart = 0;
	m_windowSize = windowSize;
	Redraw();
}
