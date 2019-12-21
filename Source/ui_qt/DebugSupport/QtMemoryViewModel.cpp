#include "QtMemoryViewModel.h"

#include "string_format.h"
#include "lexical_cast_ex.h"

std::vector<CQtMemoryViewModel::UNITINFO> CQtMemoryViewModel::g_units =
    {
        {1, 2, &CQtMemoryViewModel::RenderByteUnit, "8-bit Integers"},
        {4, 8, &CQtMemoryViewModel::RenderWordUnit, "32-bit Integers"},
        {4, 11, &CQtMemoryViewModel::RenderSingleUnit, "Single Precision Floating Point Numbers"}};

CQtMemoryViewModel::CQtMemoryViewModel(QObject* parent, getByteProto getByte, int size)
    : QAbstractTableModel(parent)
    , m_activeUnit(0)
    , m_getByte(getByte)
    , m_size(size)
{
}

CQtMemoryViewModel::~CQtMemoryViewModel()
{
}


int CQtMemoryViewModel::rowCount(const QModelIndex& /*parent*/) const
{
	return m_size / UnitsForCurrentLine();
}

int CQtMemoryViewModel::columnCount(const QModelIndex& /*parent*/) const
{
	return UnitsForCurrentLine() + 1;
}

QVariant CQtMemoryViewModel::data(const QModelIndex& index, int role) const
{
	if(role == Qt::DisplayRole)
	{
		if(index.column() < UnitsForCurrentLine())
		{
			int offset = (index.column() * g_units[m_activeUnit].bytesPerUnit);
			int address = offset + (index.row() * BytesForCurrentLine());
			return (this->*(g_units[m_activeUnit].renderer))(address).c_str();
		}
		else
		{
			int address = index.row() * BytesForCurrentLine();
			std::string res = "";
			for(auto j = 0; j < BytesForCurrentLine(); j++)
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
			auto address = section * BytesForCurrentLine();
			return ("0x" + lexical_cast_hex<std::string>(address, 8)).c_str();
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
	uint8 unitValue = GetByte(address);
	return string_format("%02X", unitValue);
}

std::string CQtMemoryViewModel::RenderWordUnit(uint32 address) const
{
	uint32 unitValue =
	    (GetByte(address + 0) << 0) |
	    (GetByte(address + 1) << 8) |
	    (GetByte(address + 2) << 16) |
	    (GetByte(address + 3) << 24);
	return string_format("%08X", unitValue);
}

std::string CQtMemoryViewModel::RenderSingleUnit(uint32 address) const
{
	uint32 unitValue =
	    (GetByte(address + 0) << 0) |
	    (GetByte(address + 1) << 8) |
	    (GetByte(address + 2) << 16) |
	    (GetByte(address + 3) << 24);
	return string_format("%+04.4e", *reinterpret_cast<const float*>(&unitValue));
}

unsigned int CQtMemoryViewModel::UnitsForCurrentLine() const
{
	return m_unitsForCurrentLine;
}

unsigned int CQtMemoryViewModel::BytesForCurrentLine() const
{
	return g_units[m_activeUnit].bytesPerUnit * m_unitsForCurrentLine;
}

void CQtMemoryViewModel::SetUnitsForCurrentLine(unsigned int size)
{
	m_unitsForCurrentLine = size;
}

unsigned int CQtMemoryViewModel::CharsPerUnit() const
{
	return g_units[m_activeUnit].charsPerUnit;
}

void CQtMemoryViewModel::SetActiveUnit(int index)
{
	m_activeUnit = index;
}

int CQtMemoryViewModel::GetActiveUnit()
{
	return m_activeUnit;
}

int CQtMemoryViewModel::GetBytesPerUnit()
{
	return g_units[m_activeUnit].bytesPerUnit;
}

void CQtMemoryViewModel::SetData(getByteProto getByte, int size)
{
	m_getByte = getByte;
	m_size = size;
	Redraw();
}
