#include "vfsmodel.h"
#include "vfsmanagerdialog.h"
#include "PS2VM_Preferences.h"

VFSModel::VFSModel(QObject* parent)
    : QAbstractTableModel(parent)
{
	SetupDevices();
}

int VFSModel::rowCount(const QModelIndex& /*parent*/) const
{
	return m_devices.size();
}

int VFSModel::columnCount(const QModelIndex& /*parent*/) const
{
	return 3;
}

QVariant VFSModel::data(const QModelIndex& index, int role) const
{
	if(role == Qt::DisplayRole)
	{
		const auto& device = m_devices.at(index.row());
		QString val;
		switch(index.column())
		{
		case 0:
			val = device->GetDeviceName();
			break;
		case 1:
			val = device->GetBindingType();
			break;
		case 2:
			val = device->GetBinding();
			break;
		}
		return QVariant(val);
	}
	return QVariant();
}

void VFSModel::SetupDevices()
{
	m_devices[0] = std::make_unique<CDirectoryDevice>("mc0", PREF_PS2_MC0_DIRECTORY);
	m_devices[1] = std::make_unique<CDirectoryDevice>("mc1", PREF_PS2_MC1_DIRECTORY);
	m_devices[2] = std::make_unique<CDirectoryDevice>("host", PREF_PS2_HOST_DIRECTORY);
	m_devices[3] = std::make_unique<CCdrom0Device>();
	m_devices[4] = std::make_unique<CDirectoryDevice>("rom0", PREF_PS2_ROM0_DIRECTORY);
}

bool VFSModel::setHeaderData(int section, Qt::Orientation orientation, const QVariant& value, int role)
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

QVariant VFSModel::headerData(int section, Qt::Orientation orientation, int role) const
{
	if(orientation == Qt::Horizontal)
	{
		if(role == Qt::DisplayRole)
		{
			if(section < m_h_header.size())
			{
				return m_h_header.at(section);
			}
		}
	}

	return QAbstractTableModel::headerData(section, orientation, role);
}

void VFSModel::DoubleClicked(const QModelIndex& index, QWidget* parent)
{
	CDevice* m_device = m_devices.at(index.row());
	m_device->RequestModification(parent);
}

void VFSModel::Save()
{
	for(int i = 0; i < m_devices.size(); i++)
	{
		m_devices.at(i)->Save();
	}
}
