#ifndef VFSMODEL_H
#define VFSMODEL_H

#include <QAbstractTableModel>
#include "VfsDevice.h"

class VFSModel : public QAbstractTableModel
{
    Q_OBJECT
public:
    VFSModel(QObject *parent);
    ~VFSModel();

    int rowCount(const QModelIndex &parent = QModelIndex()) const ;
    int columnCount(const QModelIndex &parent = QModelIndex()) const;
    QVariant data(const QModelIndex &index, int role = Qt::DisplayRole) const Q_DECL_OVERRIDE;
    void addData(const QStringList data) const;
    bool setHeaderData(int section, Qt::Orientation orientation, const QVariant &value, int role = Qt::EditRole) Q_DECL_OVERRIDE;
    void doubleClicked(const QModelIndex &index, QWidget *parent = 0);
    void Save();

protected:
    QVariant headerData(int section, Qt::Orientation orientation, int role) const Q_DECL_OVERRIDE;

private:
    CDevice::DeviceList m_devices;
    QVariantList m_h_header;

    void setupDevices();
};
#endif // VFSMODEL_H
