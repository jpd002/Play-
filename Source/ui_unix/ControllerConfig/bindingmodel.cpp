#include "bindingmodel.h"
#include "ControllerInfo.h"

#define CONFIG_PREFIX ("input")
#define CONFIG_BINDING_TYPE ("bindingtype")

CBindingModel::CBindingModel(QObject* parent)
    : QAbstractTableModel(parent)
{
}

CBindingModel::~CBindingModel()
{
}

int CBindingModel::rowCount(const QModelIndex& /*parent*/) const
{
	return PS2::CControllerInfo::MAX_BUTTONS;
}

int CBindingModel::columnCount(const QModelIndex& /*parent*/) const
{
	return 3;
}

QVariant CBindingModel::data(const QModelIndex& index, int role) const
{
	if(role == Qt::DisplayRole)
	{
		auto binding = m_inputManager->GetBinding(0, static_cast<PS2::CControllerInfo::BUTTON>(index.row()));
		if(binding != nullptr)
		{
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
				return QVariant(binding->GetBindingTypeName());
				break;
			case 2:
				return QVariant(binding->GetDescription(m_inputManager).c_str());
				break;
			}
		}
	}
	return QVariant();
}

void CBindingModel::Setup(CInputBindingManager* inputManager)
{
	m_inputManager = inputManager;
}

bool CBindingModel::setHeaderData(int section, Qt::Orientation orientation, const QVariant& value, int role)
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

QVariant CBindingModel::headerData(int section, Qt::Orientation orientation, int role) const
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

void CBindingModel::Refresh()
{
	QAbstractTableModel::layoutChanged();
}
