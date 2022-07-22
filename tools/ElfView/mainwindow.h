#pragma once

#include <QMainWindow>
#include "filesystem_def.h"

namespace Ui
{
	class MainWindow;
}

class QMdiArea;
class QMdiSubWindow;

class MainWindow : public QMainWindow
{
	Q_OBJECT

public:
	explicit MainWindow(QWidget* parent = nullptr);
	~MainWindow();

private slots:
	void on_actionOpen_triggered();
	void on_actionExit_triggered();

private:
	QMdiSubWindow* CreateElfViewFromPath(QMdiArea*, const fs::path&);

	Ui::MainWindow* ui;
};
