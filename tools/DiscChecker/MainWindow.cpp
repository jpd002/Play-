#include "MainWindow.h"
#include <QFileDialog>
#include <QDesktopServices>
#include "ui_MainWindow.h"
#include "../../Source/DiskUtils.h"
#include "../../Source/ui_qt/QtUtils.h"
#include "DiscUtils.h"
#include "string_format.h"

CMainWindow::CMainWindow(QWidget* parent)
    : QMainWindow(parent)
    , ui(new Ui::MainWindow)
{
	ui->setupUi(this);
}

void CMainWindow::on_actionOpenDiscImage_triggered()
{
	QStringList filters;
	filters.push_back(QtUtils::GetDiscImageFormatsFilter());
	filters.push_back("All files (*)");

	QFileDialog dialog(this);
	dialog.setFileMode(QFileDialog::ExistingFile);
	dialog.setNameFilters(filters);
	if(dialog.exec())
	{
		auto path = dialog.selectedFiles().first();
		try
		{
			ui->filenameEdit->clear();
			ui->discIdEdit->clear();
			ui->sectorsEdit->clear();
			ui->layerBreakEdit->clear();
			ui->checksumEdit->clear();
			FillDiscInfo(path.toStdString());
			m_currentImagePath = path.toStdString();
		}
		catch(const std::exception& exception)
		{
			auto errorMessage = QString::asprintf("Failed to open disc image: %s.", exception.what());
			ui->filenameEdit->setText(errorMessage);
		}
	}
}

void CMainWindow::on_computeChecksumButton_clicked()
{
	if(m_currentImagePath.empty()) return;
	auto opticalMedia = DiskUtils::CreateOpticalMediaFromPath(m_currentImagePath);
	uint32 checksum = DiscUtils::Checksum(opticalMedia);
	ui->checksumEdit->setText(string_format("%08X", checksum).c_str());
}

void CMainWindow::on_openRedumpButton_clicked()
{
	if(m_currentImagePath.empty()) return;
	auto discId = ui->discIdEdit->text();
	auto searchUrl = QString::asprintf("http://redump.org/discs/quicksearch/%s/", discId.toStdString().c_str());
	QDesktopServices::openUrl(QUrl(searchUrl));
}

void CMainWindow::FillDiscInfo(const fs::path& path)
{
	auto opticalMedia = DiskUtils::CreateOpticalMediaFromPath(path);
	ui->filenameEdit->setText(path.filename().string().c_str());
	std::string discId;
	if(DiskUtils::TryGetDiskId(path, &discId))
	{
		ui->discIdEdit->setText(discId.c_str());
	}
	else
	{
		ui->discIdEdit->setText("(Could not obtain disc id)");
	}
	if(opticalMedia->GetDvdIsDualLayer())
	{
		ui->layerBreakEdit->setText(string_format("%d", opticalMedia->GetDvdSecondLayerStart() + 0x10).c_str());
	}
	else
	{
		ui->layerBreakEdit->setText("(N/A)");
	}
	ui->tracksListView->setRowCount(opticalMedia->GetTrackCount());
	for(int i = 0; i < opticalMedia->GetTrackCount(); i++)
	{
		const auto& track = opticalMedia->GetTrack(i);
		ui->tracksListView->setItem(i, 0, new QTableWidgetItem(string_format("%d", track.start).c_str()));
		ui->tracksListView->setItem(i, 1, new QTableWidgetItem(string_format("%d", track.pregap).c_str()));
		ui->tracksListView->setItem(i, 2, new QTableWidgetItem(string_format("%d", track.size).c_str()));
		QPushButton* computeButton = new QPushButton("Compute");
		connect(computeButton, &QPushButton::clicked, [this, i]() {
			if(m_currentImagePath.empty()) return;
			auto opticalMedia = DiskUtils::CreateOpticalMediaFromPath(m_currentImagePath);
			uint32 checksum = DiscUtils::TrackChecksum(opticalMedia, i);
			ui->tracksListView->setItem(i, 3, new QTableWidgetItem(string_format("%08X", checksum).c_str()));
		});
		ui->tracksListView->setCellWidget(i, 4, computeButton);
	}
	auto blockProvider = opticalMedia->GetBlockProvider();
	ui->sectorsEdit->setText(string_format("%d", blockProvider->GetBlockCount()).c_str());
}
