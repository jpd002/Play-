#pragma once

#include <QDialog>
#include <QTimer>
#include "DiscUtils.h"

namespace Ui
{
	class ProgressDialog;
}

class CProgressDialog : public QDialog
{
	Q_OBJECT

public:
	explicit CProgressDialog(DiscUtils::RangeChecksumTaskPtr, QWidget* parent = nullptr);
	~CProgressDialog() = default;

private slots:
	void updateProgress();
	void on_buttonBox_rejected();

private:
	Ui::ProgressDialog* ui = nullptr;

	QTimer* m_updateTimer = nullptr;
	DiscUtils::RangeChecksumTaskPtr m_task;
};
