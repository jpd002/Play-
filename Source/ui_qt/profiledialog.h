#pragma once

#include <QDialog>

namespace Ui
{
	class ProfileDialog;
}

class
    ProfileDialog : public QDialog
{
	Q_OBJECT

public:
	explicit ProfileDialog(QWidget* parent = 0);
	~ProfileDialog();

	void updateStats(std::string);

private:
	Ui::ProfileDialog* ui;
};
