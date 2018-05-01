#include "mainwindow.h"
#include "ui_mainwindow.h"
#include "PsfLoader.h"
#include "SH_OpenAL.h"
#include "PsfTags.h"
#include "AppConfig.h"

#include <QFileDialog>
#include <QMessageBox>
#include <QDateTime>
#include <chrono>

#define PREFERENCE_UI_LASTFOLDER "ui.lastfolder"

MainWindow::MainWindow(QWidget* parent)
    : QMainWindow(parent)
    , ui(new Ui::MainWindow)
    , model(this)
{
	ui->setupUi(this);

	m_virtualMachine = new CPsfVm();
	m_virtualMachine->SetSpuHandler(&CSH_OpenAL::HandlerFactory);
	m_virtualMachine->OnNewFrame.connect([&]() { OnNewFrame(); });

	model.setHeaderData(0, Qt::Orientation::Horizontal, QVariant("Game"), Qt::DisplayRole);
	model.setHeaderData(1, Qt::Orientation::Horizontal, QVariant("Title"), Qt::DisplayRole);
	model.setHeaderData(2, Qt::Orientation::Horizontal, QVariant("Length"), Qt::DisplayRole);
	ui->tableView->setModel(&model);
	ui->tableView->horizontalHeader()->setSectionResizeMode(QHeaderView::Stretch);
	ui->tableView->verticalHeader()->setSectionResizeMode(QHeaderView::Fixed);

	CAppConfig::GetInstance().RegisterPreferenceString(PREFERENCE_UI_LASTFOLDER, QDir::homePath().toStdString().c_str());
	m_path = CAppConfig::GetInstance().GetPreferenceString(PREFERENCE_UI_LASTFOLDER);

	// used as workaround to avoid direct ui access from a thread
	connect(this, SIGNAL(ChangeRow(int)), ui->tableView, SLOT(selectRow(int)));

	m_running = true;
	m_thread = std::thread(&MainWindow::UiUpdateLoop, this);
}

void MainWindow::UiUpdateLoop()
{
	while(m_running)
	{
		auto end = std::chrono::steady_clock::now() + std::chrono::milliseconds(16);
		if(m_frames > m_trackLength)
		{
			m_frames = 0;
			on_nextButton_clicked();
		}
		else if(m_frames >= m_fadePosition)
		{
			float currentRatio = static_cast<float>(m_trackLength - m_fadePosition) / static_cast<float>(m_trackLength - m_frames);
			float currentVolume = (1 / currentRatio) * m_volumeAdjust;
			m_virtualMachine->SetVolumeAdjust(currentVolume);
		}
		else if(m_trackLength > 0 && m_frames > 0 && m_fadePosition > 0)
		{
			float currentRatio = 1 / (static_cast<float>(m_trackLength - m_fadePosition) / static_cast<float>(m_frames));
			if(currentRatio < 1.0f)
			{
				float currentVolume = currentRatio * m_volumeAdjust;
				m_virtualMachine->SetVolumeAdjust(currentVolume);
			}
		}
		std::this_thread::sleep_until(end);
	}
}

MainWindow::~MainWindow()
{
	m_running = false;
	if(m_thread.joinable()) m_thread.join();
	if(m_virtualMachine != nullptr)
	{
		m_virtualMachine->Pause();
		delete m_virtualMachine;
		m_virtualMachine = nullptr;
	}
	delete ui;
}

void MainWindow::on_actionOpen_triggered()
{
	QFileDialog dialog(this);
	dialog.setFileMode(QFileDialog::ExistingFiles);
	dialog.setDirectory(QString(m_path.c_str()));
	dialog.setNameFilter(tr("PSF files (*.psf *.psf2 *.minipsf *.minipsf2)"));
	if(dialog.exec())
	{
		if(m_virtualMachine != nullptr)
		{
			m_virtualMachine->Pause();
			m_virtualMachine->Reset();
			int index = model.rowCount();
			foreach(QString file, dialog.selectedFiles())
			{
				try
				{
					CPsfBase::TagMap tags;
					std::string fileName = file.toStdString();
					CPsfLoader::LoadPsf(*m_virtualMachine, fileName, "", &tags);
					model.addPlaylistItem(fileName, tags);
				}
				catch(const std::exception& e)
				{
					QMessageBox messageBox;
					messageBox.critical(0, "Error", e.what());
					messageBox.show();
				}
			}

			m_path = QFileInfo(model.at(index)->path.c_str()).absolutePath().toStdString();
			CAppConfig::GetInstance().SetPreferenceString(PREFERENCE_UI_LASTFOLDER, m_path.c_str());
			PlayTrackIndex(index);
		}
	}
}

void MainWindow::UpdateTrackDetails(CPsfBase::TagMap& tags)
{
	auto tag = CPsfTags(tags);
	ui->current_game->setText(QString::fromWCharArray(tag.GetTagValue("game").c_str()));
	ui->current_total_length->setText(QString::fromWCharArray(tag.GetTagValue("length").c_str()));
	ui->current_title->setText(QString::fromWCharArray(tag.GetTagValue("title").c_str()));
	ui->current_artist->setText(QString::fromWCharArray(tag.GetTagValue("artist").c_str()));
	ui->current_cp->setText(QString::fromWCharArray(tag.GetTagValue("copyright").c_str()));
	ui->current_year->setText(QString::fromWCharArray(tag.GetTagValue("year").c_str()));

	m_volumeAdjust = 1.0f;

	try
	{
		m_volumeAdjust = stof(tag.GetTagValue("volume"));
	}
	catch(...)
	{
	}
	double dlength = CPsfTags::ConvertTimeString(tag.GetTagValue("length").c_str());
	double dfade = CPsfTags::ConvertTimeString(tag.GetTagValue("fade").c_str());
	m_trackLength = static_cast<uint64>(dlength * 60.0);
	m_fadePosition = m_trackLength - static_cast<uint64>(dfade * 60.0);
	m_frames = 0;

	m_virtualMachine->SetVolumeAdjust((dfade > 0) ? 0 : m_volumeAdjust);

	ChangeRow(m_currentindex);
}

void MainWindow::OnNewFrame()
{
	m_frames++;
	ui->current_time->setText(QDateTime::fromTime_t(m_frames / 60).toUTC().toString("mm:ss"));
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

void MainWindow::PlayTrackIndex(int index)
{
	m_virtualMachine->Pause();
	m_virtualMachine->Reset();
	m_currentindex = index;
	CPsfBase::TagMap tags;
	CPsfLoader::LoadPsf(*m_virtualMachine, model.at(m_currentindex)->path, "", &tags);
	UpdateTrackDetails(tags);
	m_virtualMachine->Resume();
}

void MainWindow::on_tableView_doubleClicked(const QModelIndex& index)
{
	PlayTrackIndex(index.row());
}

void MainWindow::DeleteTrackIndex(int index)
{
	auto playlistsize = model.removePlaylistItem(index);
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
