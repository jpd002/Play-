#include "QtDialogListWidget.h"
#include "ui_QtDialogListWidget.h"

QtDialogListWidget::QtDialogListWidget(QWidget* parent)
    : QDialog(parent)
    , ui(new Ui::QtDialogListWidget)
{
	ui->setupUi(this);

	connect(ui->listWidget, &QListWidget::itemDoubleClicked, this, &QtDialogListWidget::on_buttonBox_accepted);
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
	auto item = ui->listWidget->currentItem();
	if(item)
	{
		results = item->text().toStdString();
	}
	QDialog::accept();
}
