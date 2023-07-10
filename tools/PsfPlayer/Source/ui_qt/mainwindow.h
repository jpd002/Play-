#pragma once

#include <QMainWindow>
#include "PsfVm.h"
#include "PsfBase.h"
#include "PlaybackController.h"
#include "playlistmodel.h"
#include <thread>
#include "filesystem_def.h"

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

	void AddFileToPlaylist(const fs::path&);
	void AddArchiveToPlaylist(const fs::path&);

private:
	void UpdateTrackDetails(CPsfBase::TagMap&);
	void OnNewFrame();

	void PlayTrackIndex(int index);
	void DeleteTrackIndex(int index);

	Ui::MainWindow* ui;
	CPsfVm* m_virtualMachine;
	PlaylistModel model;
	int m_currentindex = -1;
	CPlaybackController m_playbackController;

	fs::path m_path;

	Framework::CSignal<void()>::Connection m_OnNewFrameConnection;

	Framework::CSignal<void(float)>::Connection m_PlaybackVolumeChangedConnection;
	Framework::CSignal<void()>::Connection m_PlaybackCompletedConnection;

signals:
	void playbackCompleted();

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
	void on_playbackCompleted();
};
