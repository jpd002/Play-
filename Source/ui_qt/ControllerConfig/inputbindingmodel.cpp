#include "inputbindingmodel.h"
#include "ControllerInfo.h"
#include "../input/InputBindingManager.h"

#define CONFIG_PREFIX ("input")
#define CONFIG_BINDING_TYPE ("bindingtype")

CInputBindingModel::CInputBindingModel(QObject* parent, CInputBindingManager* inputManager, uint32 padIndex)
    : QAbstractTableModel(parent)
    , m_inputManager(inputManager)
    , m_padIndex(padIndex)
{
}

int CInputBindingModel::rowCount(const QModelIndex& /*parent*/) const
{
	return PS2::CControllerInfo::MAX_BUTTONS;
}

int CInputBindingModel::columnCount(const QModelIndex& /*parent*/) const
{
	return 3;
}

QVariant CInputBindingModel::data(const QModelIndex& index, int role) const
{
	if(role == Qt::DisplayRole)
	{
		auto binding = m_inputManager->GetBinding(m_padIndex, static_cast<PS2::CControllerInfo::BUTTON>(index.row()));
		switch(index.column())
		{
		case 0:
		{
			std::string str(PS2::CControllerInfo::m_buttonName[index.row()]);
			std::transform(str.begin(), str.end(), str.begin(), ::toupper);
			return QVariant(str.c_str());
		}
		break;
		case 1:
			return binding ? QVariant(binding->GetBindingTypeName()) : QVariant();
			break;
		case 2:
			return binding ? QVariant(binding->GetDescription(m_inputManager).c_str()) : QVariant();
			break;
		}
	}
	return QVariant();
}

bool CInputBindingModel::setHeaderData(int section, Qt::Orientation orientation, const QVariant& value, int role)
{
	if(orientation == Qt::Horizontal)
	{
		if(role == Qt::DisplayRole)
		{
			m_h_header.insert(section, value);
			return true;
		}
	}
	return QAbstractTableModel::setHeaderData(section, orientation, value, role);
}

QVariant CInputBindingModel::headerData(int section, Qt::Orientation orientation, int role) const
{
	if(orientation == Qt::Horizontal)
	{
		if(role == Qt::DisplayRole)
		{
			if(section < m_h_header.size())
				return m_h_header.at(section);
		}
	}

	return QAbstractTableModel::headerData(section, orientation, role);
}

void CInputBindingModel::Refresh()
{
	QAbstractTableModel::layoutChanged();
}

void CInputBindingModel::RefreshRow(const QModelIndex& modelIndex)
{
	emit QAbstractTableModel::dataChanged(index(modelIndex.row(), 0), index(modelIndex.row(), columnCount()));
}
