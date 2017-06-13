#pragma once

#include <QAbstractTableModel>
#include <GamePad/GamePadDeviceListener.h>
#include "ui_unix/PH_HidUnix.h"

class CBindingModel : public QAbstractTableModel
{
	Q_OBJECT
public:
	CBindingModel(QObject *parent);
	~CBindingModel();

        int			rowCount(const QModelIndex &parent = QModelIndex()) const Q_DECL_OVERRIDE;
        int			columnCount(const QModelIndex &parent = QModelIndex()) const Q_DECL_OVERRIDE;
        QVariant		data(const QModelIndex &index, int role = Qt::DisplayRole) const Q_DECL_OVERRIDE;
        void			addData(const QStringList data) const;
        bool			setHeaderData(int section, Qt::Orientation orientation, const QVariant &value, int role = Qt::EditRole) Q_DECL_OVERRIDE;
        void			Refresh();

        void			Setup(CInputBindingManager*);

protected:
        QVariant		headerData(int section, Qt::Orientation orientation, int role) const Q_DECL_OVERRIDE;

private:
        CInputBindingManager*		m_inputManager;
        QVariantList			m_h_header;

};
