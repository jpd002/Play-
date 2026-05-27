#include "ProgressDialog.h"
#include "ui_ProgressDialog.h"

CProgressDialog::CProgressDialog(DiscUtils::RangeChecksumTaskPtr task, QWidget* parent)
    : QDialog(parent)
    , ui(new Ui::ProgressDialog)
    , m_task(task)
{
	ui->setupUi(this);
	m_updateTimer = new QTimer(this);
	connect(m_updateTimer, SIGNAL(timeout()), this, SLOT(updateProgress()));
	m_updateTimer->start(50);
}

void CProgressDialog::on_buttonBox_rejected()
{
	m_task->cancelFlag = true;
	close();
}

void CProgressDialog::updateProgress()
{
	ui->progressBar->setValue(m_task->progress * 100.f);
	if(m_task->progress == 1.0f)
	{
		close();
	}
}
