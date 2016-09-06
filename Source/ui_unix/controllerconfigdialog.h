#ifndef CONTROLLERCONFIGDIALOG_H
#define CONTROLLERCONFIGDIALOG_H

#include <QDialog>
#include <QAbstractButton>
#include <QXmlStreamReader>
#include "PH_HidUnix.h"

namespace Ui {
class ControllerConfigDialog;
}

class ControllerConfigDialog : public QDialog
{
    Q_OBJECT

public:
    explicit ControllerConfigDialog(QWidget *parent = 0);
    ~ControllerConfigDialog();
    static CPH_HidUnix::BindingPtr *GetBinding(int);
    static QString ReadElementValue(QXmlStreamReader &Rxml);

private slots:
    void on_buttonBox_clicked(QAbstractButton *button);

private:
    void Save(int index, QWidget *tab);
    void Load(int index, QWidget *tab);
    Ui::ControllerConfigDialog *ui;
};

#endif // CONTROLLERCONFIGDIALOG_H
