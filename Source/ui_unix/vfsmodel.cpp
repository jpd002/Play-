#include "vfsmodel.h"
#include "vfsmanagerdialog.h"

VFSModel::VFSModel(QObject *parent)
    :QAbstractTableModel(parent)
{
    SetupDevices();
}

VFSModel::~VFSModel()
{
    for (int i = 0; i < m_devices.size(); i++ )
    {
        delete m_devices.at(i);
    }
}

int VFSModel::rowCount(const QModelIndex & /*parent*/) const
{
    return m_devices.size();
}

int VFSModel::columnCount(const QModelIndex & /*parent*/) const
{
    if (m_devices.size() > 0) {
        return 3;
    } else {
        return 0;
    }
}

QVariant VFSModel::data(const QModelIndex &index, int role) const
{
    if (role == Qt::DisplayRole)
    {
        CDevice* m_device = m_devices.at(index.row());
        const char* val = "";
        switch (index.column())
        {
        case 0:
            val = m_device->GetDeviceName();
            break;
        case 1:
            val = m_device->GetBindingType();
            break;
        case 2:
            val = m_device->GetBinding();
            break;
        }
        return QVariant(val);
    }
    return QVariant();
}

void VFSModel::SetupDevices()
{
    m_devices[0] = new CDirectoryDevice("mc0",  "ps2.mc0.directory");
    m_devices[1] = new CDirectoryDevice("mc1",  "ps2.mc1.directory");
    m_devices[2] = new CDirectoryDevice("host", "ps2.host.directory");
    m_devices[3] = new CCdrom0Device();
}

bool VFSModel::setHeaderData(int section, Qt::Orientation orientation, const QVariant &value, int role)
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

QVariant VFSModel::headerData(int section, Qt::Orientation orientation, int role) const
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

void VFSModel::DoubleClicked(const QModelIndex &index, QWidget* parent)
{
    CDevice* m_device = m_devices.at(index.row());
    m_device->RequestModification(parent);
}

void VFSModel::Save()
{
    for (int i = 0; i < m_devices.size(); i++ )
    {
        m_devices.at(i)->Save();
    }
}
