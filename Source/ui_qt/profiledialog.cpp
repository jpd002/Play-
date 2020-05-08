#include "profiledialog.h"
#include "ui_profiledialog.h"

ProfileDialog::ProfileDialog(QWidget* parent)
    : QDialog(parent)
    , ui(new Ui::ProfileDialog)
{
	ui->setupUi(this);
}

ProfileDialog::~ProfileDialog()
{
	delete ui;
}

void ProfileDialog::updateStats(std::string msg)
{
	ui->profileStatsLabel->setText(msg.c_str());
}
