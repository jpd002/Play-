#include "bindingmodel.h"
#include "vfsmanagerdialog.h"
#include "ControllerInfo.h"
#include "AppConfig.h"
#include "PH_HidUnix.h"
#include "inputeventselectiondialog.h"
#include "ControllerInfo.h"

#define CONFIG_PREFIX						("input")
#define CONFIG_BINDING_TYPE					("bindingtype")

CBindingModel::CBindingModel(QObject *parent)
	:QAbstractTableModel(parent)
{
}

CBindingModel::~CBindingModel()
{
}

int CBindingModel::rowCount(const QModelIndex & /*parent*/) const
{
	return PS2::CControllerInfo::MAX_BUTTONS;
}

int CBindingModel::columnCount(const QModelIndex & /*parent*/) const
{
	return 3;
}

QVariant CBindingModel::data(const QModelIndex &index, int role) const
{
	if (role == Qt::DisplayRole)
	{
		auto binding = m_inputManager->GetBinding(static_cast<PS2::CControllerInfo::BUTTON>(index.row()));
		if (binding != nullptr)
		{
			const char* val = "";
			switch (index.column())
			{
			case 0:
				val = PS2::CControllerInfo::m_buttonName[index.row()];
			break;
			case 1:
				val = binding->GetBindingTypeName();
			break;
			case 2:
				val = binding->GetDescription().c_str();
			break;
			}
			return QVariant(val);
		}
	}
	return QVariant();
}

void CBindingModel::Setup(CInputBindingManager* inputManager)
{
	m_inputManager = inputManager;
}

bool CBindingModel::setHeaderData(int section, Qt::Orientation orientation, const QVariant &value, int role)
{
	if (orientation == Qt::Horizontal)
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
	if (orientation == Qt::Horizontal)
	{
		if(role == Qt::DisplayRole)
		{
			if (section < m_h_header.size())
				return m_h_header.at(section);
		}
	}

	return QAbstractTableModel::headerData(section, orientation, role);
}

void CBindingModel::DoubleClicked(const QModelIndex &index, QWidget* parent)
{

	InputEventSelectionDialog IESD;
	IESD.Setup(PS2::CControllerInfo::m_buttonName[index.row()], m_inputManager, static_cast<PS2::CControllerInfo::BUTTON>(index.row()));
	IESD.exec();
}


void CBindingModel::Refresh()
{
	QAbstractTableModel::layoutChanged();
}
