#pragma once

#include <QMainWindow>
#include "PsfVm.h"
#include "PsfBase.h"
#include "PlaylistItem.h"
#include "playlistmodel.h"
#include <thread>

namespace Ui
{
	class MainWindow;
}

class MainWindow : public QMainWindow
{
	Q_OBJECT

public:
	explicit MainWindow(QWidget* parent = 0);
	~MainWindow();

private:
	void UiUpdateLoop();
	void UpdateTrackDetails(CPsfBase::TagMap&);
	void OnNewFrame();

	void PlayTrackIndex(int index);
	void DeleteTrackIndex(int index);

	Ui::MainWindow* ui;
	CPsfVm* m_virtualMachine;
	PlaylistModel model;
	int m_currentindex = -1;
	uint64 m_trackLength = 10;
	uint64 m_frames = 0;
	uint64 m_fadePosition = 1;
	float m_volumeAdjust;

	std::thread m_thread;
	std::atomic<bool> m_running;
	std::string m_path;

Q_SIGNALS:
	void ChangeRow(int);

private slots:
	void on_actionOpen_triggered();
	void on_tableView_doubleClicked(const QModelIndex& index);
	void on_playpauseButton_clicked();
	void on_nextButton_clicked();
	void on_prevButton_clicked();
	void on_tableView_customContextMenuRequested(const QPoint& pos);
	void on_actionPlayPause_triggered();
	void on_actionPrev_triggered();
	void on_actionNext_triggered();
};
