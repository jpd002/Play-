#pragma once

#include <QMainWindow>
#include "filesystem_def.h"

namespace Ui
{
	class MainWindow;
}

class CMainWindow : public QMainWindow
{
	Q_OBJECT
public:
	explicit CMainWindow(QWidget* parent = nullptr);
	~CMainWindow() = default;

private slots:
	void on_actionOpenDiscImage_triggered();
	void on_computeChecksumButton_clicked();
	void on_openRedumpButton_clicked();

private:
	void FillDiscInfo(const fs::path&);

	Ui::MainWindow* ui = nullptr;
	fs::path m_currentImagePath;
};
