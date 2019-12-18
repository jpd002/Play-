#include "QtDialogListWidget.h"
#include "ui_QtDialogListWidget.h"

QtDialogListWidget::QtDialogListWidget(QWidget *parent) :
    QDialog(parent),
    ui(new Ui::QtDialogListWidget)
{
	ui->setupUi(this);
}

QtDialogListWidget::~QtDialogListWidget()
{
	delete ui;
}

void QtDialogListWidget::addItem(std::string item)
{
	ui->listWidget->addItem(item.c_str());
}

std::string QtDialogListWidget::getResult()
{
	return results;
}

void QtDialogListWidget::on_buttonBox_accepted()
{
	results = ui->listWidget->currentItem()->text().toStdString();
}
