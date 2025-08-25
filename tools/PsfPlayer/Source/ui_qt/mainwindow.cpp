#include "mainwindow.h"
#include "ui_mainwindow.h"
#include "ui_debugmenu.h"
#include "PsfLoader.h"
#include "PsfTags.h"
#include "AppConfig.h"
#include "../../../../Source/ui_qt/QStringUtils.h"

#ifdef WIN32
#include "sound/SH_WaveOut/SH_WaveOut.h"
#else
#include "sound/SH_OpenAL/SH_OpenAL.h"
#endif

#ifdef DEBUGGER_INCLUDED
#include "debuggerwindow.h"
#endif

#include <QFileDialog>
#include <QMessageBox>
#include <QDateTime>
#include <chrono>

#define PREFERENCE_UI_LASTFOLDER "ui.lastfolder.v2"

MainWindow::MainWindow(QWidget* parent)
    : QMainWindow(parent)
    , ui(new Ui::MainWindow)
    , model(this)
{
	ui->setupUi(this);

	m_virtualMachine = new CPsfVm();
#ifdef WIN32
	m_virtualMachine->SetSpuHandler(&CSH_WaveOut::HandlerFactory);
#else
	m_virtualMachine->SetSpuHandler(&CSH_OpenAL::HandlerFactory);
#endif
	m_OnNewFrameConnection = m_virtualMachine->OnNewFrame.Connect([&]() { OnNewFrame(); });

	model.setHeaderData(0, Qt::Orientation::Horizontal, QVariant("Title"), Qt::DisplayRole);
	model.setHeaderData(1, Qt::Orientation::Horizontal, QVariant("Length"), Qt::DisplayRole);

	SetupDebugger();

	ui->tableView->setModel(&model);
	ui->tableView->horizontalHeader()->setSectionResizeMode(QHeaderView::Stretch);
	ui->tableView->verticalHeader()->setSectionResizeMode(QHeaderView::Fixed);

	m_PlaybackVolumeChangedConnection = m_playbackController.VolumeChanged.Connect(
	    [this](float volume) { m_virtualMachine->SetVolumeAdjust(volume); });
	m_PlaybackCompletedConnection = m_playbackController.PlaybackCompleted.Connect(
	    [this]() { emit playbackCompleted(); });

	auto homePath = QStringToPath(QDir::homePath());
	CAppConfig::GetInstance().RegisterPreferencePath(PREFERENCE_UI_LASTFOLDER, homePath);
	m_path = CAppConfig::GetInstance().GetPreferencePath(PREFERENCE_UI_LASTFOLDER);

	connect(this, SIGNAL(playbackCompleted()), this, SLOT(on_playbackCompleted()));
}

MainWindow::~MainWindow()
{
#ifdef DEBUGGER_INCLUDED
	m_debugger.reset();
#endif
	if(m_virtualMachine != nullptr)
	{
		m_virtualMachine->Pause();
		delete m_virtualMachine;
		m_virtualMachine = nullptr;
	}
	delete ui;
#ifdef DEBUGGER_INCLUDED
	delete debugMenuUi;
#endif
}

void MainWindow::SetupDebugger()
{
#ifdef DEBUGGER_INCLUDED
	m_debugger = std::make_unique<DebuggerWindow>(*m_virtualMachine);

	{
		auto debugMenu = new QMenu(this);
		debugMenuUi = new Ui::DebugMenu();
		debugMenuUi->setupUi(debugMenu);
		ui->menuBar->insertMenu(nullptr, debugMenu);

		connect(debugMenuUi->actionShowDebugger, &QAction::triggered, this, std::bind(&MainWindow::ShowDebugger, this));
	}
#endif
}

void MainWindow::on_actionOpen_triggered()
{
	QStringList filters;
	filters.push_back(tr("All supported files (*.psf *.psf2 *.psfp *.minipsf *.minipsf2 *.minipsfp *.zip; *.rar)"));
	filters.push_back(tr("PSF files (*.psf *.psf2 *.psfp *.minipsf *.minipsf2 *.minipsfp)"));
	filters.push_back(tr("PSF archives (*.zip; *.rar)"));

	QFileDialog dialog(this);
	dialog.setFileMode(QFileDialog::ExistingFile);
	dialog.setDirectory(PathToQString(m_path));
	dialog.setNameFilters(filters);

	if(dialog.exec())
	{
		if(m_virtualMachine)
		{
			m_virtualMachine->Pause();
			m_virtualMachine->Reset();
		}

		auto file = QStringToPath(dialog.selectedFiles().first());

		m_path = file.parent_path();
		CAppConfig::GetInstance().SetPreferencePath(PREFERENCE_UI_LASTFOLDER, m_path);

		model.clearPlaylist();

		auto extension = file.extension();
		if(extension == ".zip" || extension == ".rar")
		{
			AddArchiveToPlaylist(file);
		}
		else
		{
			AddFileToPlaylist(file);
		}

		PlayTrackIndex(0);
	}
}

void MainWindow::AddFileToPlaylist(const fs::path& filePath)
{
	try
	{
		model.addItem(filePath);
	}
	catch(const std::exception& e)
	{
		QMessageBox messageBox;
		messageBox.critical(0, "Error", e.what());
		messageBox.show();
	}
}

void MainWindow::AddArchiveToPlaylist(const fs::path& archivePath)
{
	try
	{
		model.addArchive(archivePath);
	}
	catch(const std::exception& e)
	{
		QMessageBox messageBox;
		messageBox.critical(0, "Error", e.what());
		messageBox.show();
	}
}

#ifdef DEBUGGER_INCLUDED

void MainWindow::ShowDebugger()
{
	m_debugger->showMaximized();
	m_debugger->raise();
	m_debugger->activateWindow();
}

#endif

void MainWindow::UpdateTrackDetails(CPsfBase::TagMap& tags)
{
	auto tag = CPsfTags(tags);

	ui->current_game->setText(QString::fromWCharArray(tag.GetTagValue("game").c_str()));
	ui->current_total_length->setText(QString::fromWCharArray(tag.GetTagValue("length").c_str()));
	ui->current_title->setText(QString::fromWCharArray(tag.GetTagValue("title").c_str()));
	ui->current_artist->setText(QString::fromWCharArray(tag.GetTagValue("artist").c_str()));
	ui->current_cp->setText(QString::fromWCharArray(tag.GetTagValue("copyright").c_str()));
	ui->current_year->setText(QString::fromWCharArray(tag.GetTagValue("year").c_str()));

	ui->tableView->selectRow(m_currentindex);

	m_playbackController.Play(tag);
}

void MainWindow::on_playbackCompleted()
{
	on_nextButton_clicked();
}

void MainWindow::OnNewFrame()
{
	m_playbackController.Tick();
	ui->current_time->setText(QDateTime::fromSecsSinceEpoch(m_playbackController.GetFrameCount() / 60).toUTC().toString("mm:ss"));
}

void MainWindow::on_tableView_customContextMenuRequested(const QPoint& pos)
{
	auto item = ui->tableView->indexAt(pos);
	if(item.isValid())
	{
		QMenu* menu = new QMenu(this);
		auto playAct = new QAction("Play", this);
		auto delAct = new QAction("Remove", this);

		menu->addAction(playAct);
		menu->addAction(delAct);
		menu->popup(ui->tableView->viewport()->mapToGlobal(pos));

		auto index = item.row();
		connect(playAct, &QAction::triggered, std::bind(&MainWindow::PlayTrackIndex, this, index));
		connect(delAct, &QAction::triggered, std::bind(&MainWindow::DeleteTrackIndex, this, index));
	}
}

void MainWindow::UnloadCurrentTrack()
{
	if(m_currentindex == -1)
	{
		return;
	}
	m_virtualMachine->Pause();
#ifdef DEBUGGER_INCLUDED
	auto item = model.at(m_currentindex);
	auto itemPath = fs::path(item->path);
	auto tagPackageName = itemPath.stem().string();
	m_virtualMachine->SaveDebugTags(tagPackageName.c_str());
#endif
	m_currentindex = -1;
}

void MainWindow::PlayTrackIndex(int index)
{
	UnloadCurrentTrack();
	m_virtualMachine->Reset();
	m_currentindex = index;

	CPsfBase::TagMap tags;
	auto item = model.at(m_currentindex);
	auto archivePath = model.getArchivePath(item->archiveId);
	CPsfLoader::LoadPsf(*m_virtualMachine, item->path, archivePath, &tags);
	UpdateTrackDetails(tags);

#ifdef DEBUGGER_INCLUDED
	auto itemPath = fs::path(item->path);
	auto tagPackageName = itemPath.stem().string();
	m_virtualMachine->LoadDebugTags(tagPackageName.c_str());
	m_debugger->Reset();
#else
	m_virtualMachine->Resume();
#endif
}

void MainWindow::closeEvent(QCloseEvent* event)
{
	UnloadCurrentTrack();
#ifdef DEBUGGER_INCLUDED
	m_debugger->close();
#endif
}

void MainWindow::on_tableView_doubleClicked(const QModelIndex& index)
{
	PlayTrackIndex(index.row());
}

void MainWindow::DeleteTrackIndex(int index)
{
	auto playlistsize = model.removeItem(index);
	if(index == m_currentindex)
	{
		if(playlistsize >= m_currentindex)
		{
			PlayTrackIndex(m_currentindex);
		}
		else if(m_currentindex > 0 && playlistsize == m_currentindex - 1)
		{
			PlayTrackIndex(--m_currentindex);
		}
		else
		{
			if(m_virtualMachine->GetStatus() == CVirtualMachine::STATUS::RUNNING)
			{
				m_virtualMachine->Pause();
				m_virtualMachine->Reset();
			}
			m_currentindex--;
		}
	}
	else if(index < m_currentindex)
	{
		m_currentindex--;
	}
}

void MainWindow::on_playpauseButton_clicked()
{
	if(m_currentindex > -1)
	{
		if(m_virtualMachine != nullptr)
		{
			if(m_virtualMachine->GetStatus() == CVirtualMachine::STATUS::RUNNING)
			{
				m_virtualMachine->Pause();
			}
			else
			{
				m_virtualMachine->Resume();
			}
		}
	}
	else
	{
		on_actionOpen_triggered();
	}
}

void MainWindow::on_nextButton_clicked()
{
	if(model.rowCount() < 1) return;

	int currentindex = m_currentindex;
	if(m_currentindex < model.rowCount() - 1)
	{
		m_currentindex += 1;
	}
	else
	{
		m_currentindex = 0;
	}

	if(currentindex == m_currentindex) return;

	PlayTrackIndex(m_currentindex);
}

void MainWindow::on_prevButton_clicked()
{
	if(model.rowCount() < 1) return;

	if(m_currentindex > 0)
	{
		m_currentindex -= 1;
	}
	else
	{
		m_currentindex = model.rowCount() - 1;
	}

	PlayTrackIndex(m_currentindex);
}

void MainWindow::on_actionPlayPause_triggered()
{
	on_playpauseButton_clicked();
}

void MainWindow::on_actionPrev_triggered()
{
	on_prevButton_clicked();
}

void MainWindow::on_actionNext_triggered()
{
	on_nextButton_clicked();
}
