#pragma once

#include <QDialog>

namespace Ui
{
	class QtDialogListWidget;
}

class QtDialogListWidget : public QDialog
{
	Q_OBJECT

public:
	explicit QtDialogListWidget(QWidget* parent = nullptr);
	~QtDialogListWidget();

	void addItem(std::string);
	std::string getResult();

private slots:
	void on_buttonBox_accepted();

private:
	Ui::QtDialogListWidget* ui;

	std::string results;
};
