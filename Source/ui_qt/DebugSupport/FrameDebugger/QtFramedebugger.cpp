#include "ui_QtFramedebugger.h"
#include "QtFramedebugger.h"

#include "../../openglwindow.h"
#include "filesystem_def.h"
#include "AppConfig.h"
#include "StdStreamUtils.h"
#include "string_cast.h"
#include "string_format.h"
#include "GsPacketData.h"
#include "GsPacketListModel.h"
#include "GsStateUtils.h"
#include "DebuggerDefaults.h"

#include <QMessageBox>
#include <QFileDialog>
#include <QOffscreenSurface>
#include <QApplication>

#define PREF_FRAMEDEBUGGER_FRAMEBUFFER_DISPLAYMODE "framedebugger.framebuffer.displaymode"

QtFramedebugger::QtFramedebugger()
    : ui(new Ui::QtFramedebugger)
{
	ui->setupUi(this);

	QFont fixedFont = QFont(DEBUGGER_DEFAULT_MONOSPACE_FONT_FACE_NAME, DEBUGGER_DEFAULT_MONOSPACE_FONT_SIZE);
	ui->inputStateTextEdit->setFont(fixedFont);

	OpenGLWindow openglpanel;
	QOffscreenSurface* surface = new QOffscreenSurface();
	surface->setFormat(OpenGLWindow::GetSurfaceFormat());
	surface->create();
	m_gs = std::make_unique<CGSH_OpenGLQt>(surface);
	m_gs->SetLoggingEnabled(false);
	m_gs->Initialize();
	m_gs->Reset();
	// CHECKGLERROR();

	m_gsContextView0 = std::make_unique<CGsContextView>(this, ui->context0Buffer, ui->fitContext1Button, ui->saveContext1, m_gs.get(), 0);
	m_gsContextView1 = std::make_unique<CGsContextView>(this, ui->context1Buffer, ui->fitContext2Button, ui->saveContext2, m_gs.get(), 1);

	m_vu1ProgramView = std::make_unique<CVu1ProgramView>(this, m_vu1vm);

	ui->context0_layout->addWidget(m_gsContextView0.get(), 0, 0);
	ui->context1_layout->addWidget(m_gsContextView1.get(), 0, 0);
	ui->gridLayout_10->addWidget(m_vu1ProgramView.get(), 0, 0);

	m_gsContextView0->SetFbDisplayMode(m_fbDisplayMode);
	m_gsContextView1->SetFbDisplayMode(m_fbDisplayMode);

	ui->actionAlpha_Test_Enabled->setChecked(true);
	ui->actionDepth_Test_Enabled->setChecked(true);
	ui->actionAlpha_Blend_Enabled->setChecked(true);

	auto alignmentGroup = new QActionGroup(this);
	alignmentGroup->addAction(ui->actionRaw);
	alignmentGroup->addAction(ui->action640x448_Non_Interlaced);
	alignmentGroup->addAction(ui->action640x448_Interlaced);
	ui->actionRaw->setChecked(true);
}

QtFramedebugger::~QtFramedebugger()
{
	delete ui;
}

void QtFramedebugger::showEvent(QShowEvent* event)
{
	QMainWindow::showEvent(event);
}

void QtFramedebugger::LoadFrameDump(std::string path)
{
	try
	{
		fs::path dumpPath(path);
		auto inputStream = Framework::CreateInputStdStream(dumpPath.native());
		m_frameDump.Read(inputStream);
		m_frameDump.IdentifyDrawingKicks();
	}
	catch(const std::exception& exception)
	{
		std::string message = string_format("Failed to open frame dump:\r\n\r\n%s", exception.what());
		QMessageBox msgBox;
		msgBox.setText(message.c_str());
		msgBox.exec();
		return;
	}

	m_vu1vm.Reset();

	auto model = new PacketTreeModel(this);
	model->setupModelData(m_frameDump);
	ui->treeView->setModel(model);
	connect(ui->treeView->selectionModel(), &QItemSelectionModel::selectionChanged, this, &QtFramedebugger::selectionChanged);

	UpdateDisplay(0);
}

void QtFramedebugger::UpdateDisplay(int32 targetCmdIndex)
{
	m_gs->Reset();

	uint8* gsRam = m_gs->GetRam();
	uint64* gsRegisters = m_gs->GetRegisters();
	memcpy(gsRam, m_frameDump.GetInitialGsRam(), CGSHandler::RAMSIZE);
	memcpy(gsRegisters, m_frameDump.GetInitialGsRegisters(), CGSHandler::REGISTER_MAX * sizeof(uint64));
	m_gs->SetSMODE2(m_frameDump.GetInitialSMODE2());

	int32 cmdIndex = 0;
	for(const auto& packet : m_frameDump.GetPackets())
	{
		if((cmdIndex - 1) >= targetCmdIndex) break;

		m_currentMetadata = packet.metadata;

		if(packet.registerWrites.empty())
		{
			m_gs->ProcessWriteBuffer(nullptr);
			m_gs->FeedImageData(packet.imageData.data(), packet.imageData.size());
		}
		else
		{
			for(const auto& registerWrite : packet.registerWrites)
			{
				if((cmdIndex - 1) >= targetCmdIndex) break;
				m_gs->WriteRegister(registerWrite);
				cmdIndex++;
			}
		}
	}

	m_gs->ProcessWriteBuffer(nullptr);
	m_gs->Finish();

	const auto& drawingKicks = m_frameDump.GetDrawingKicks();
	if(!drawingKicks.empty())
	{
		auto prevKickIndexIterator = drawingKicks.lower_bound(targetCmdIndex);
		if((prevKickIndexIterator == std::end(drawingKicks)) || (prevKickIndexIterator->first != targetCmdIndex))
		{
			prevKickIndexIterator = std::prev(prevKickIndexIterator);
		}
		if(prevKickIndexIterator != std::end(drawingKicks))
		{
			m_currentDrawingKick = prevKickIndexIterator->second;
		}
		else
		{
			m_currentDrawingKick = DRAWINGKICK_INFO();
		}
	}
	else
	{
		m_currentDrawingKick = DRAWINGKICK_INFO();
	}
	UpdateCurrentTab();
}

void QtFramedebugger::SetFbDisplayMode(CGsContextView::FB_DISPLAY_MODE fbDisplayMode)
{
	m_gsContextView0->SetFbDisplayMode(fbDisplayMode);
	m_gsContextView1->SetFbDisplayMode(fbDisplayMode);
	m_fbDisplayMode = fbDisplayMode;
	CAppConfig::GetInstance().SetPreferenceInteger(PREF_FRAMEDEBUGGER_FRAMEBUFFER_DISPLAYMODE, fbDisplayMode);
}

void QtFramedebugger::StepVu1()
{
#ifdef DEBUGGER_INCLUDED
	if(m_currentMetadata.pathIndex != 1)
	{
		QApplication::beep();
		return;
	}

	const int vu1TabIndex = 3;
	if(ui->tabWidget->currentIndex() != vu1TabIndex)
	{
		ui->tabWidget->setCurrentIndex(vu1TabIndex);
	}

	m_vu1ProgramView->StepVu1();
#endif
}

void QtFramedebugger::on_nextKickButton_clicked()
{
	auto selectionMode = ui->treeView->selectionModel();
	auto packetTreeModel = ui->treeView->model();
	if(!packetTreeModel || !selectionMode)
	{
		return;
	}

	unsigned int selectedItemIndex = 0;
	auto cmdindex = GetCurrentCmdIndex();
	if(cmdindex != -1)
	{
		selectedItemIndex = cmdindex;
	}

	const auto& drawingKicks = m_frameDump.GetDrawingKicks();
	auto nextKickIndexIterator = drawingKicks.upper_bound(selectedItemIndex);
	if(nextKickIndexIterator == std::end(drawingKicks))
	{
		//Nothing to do here
		return;
	}
	auto nextCmdIndex = nextKickIndexIterator->first;
	auto kicks = static_cast<PacketTreeModel*>(packetTreeModel)->GetDrawKickIndexes();
	for(const auto& indexes : kicks)
	{
		if(indexes.cmdIndex == nextCmdIndex)
		{
			auto parentIndex = packetTreeModel->index(indexes.parentIndex, 0);
			auto childIndex = packetTreeModel->index(indexes.childIndex, 0, parentIndex);
			ui->treeView->expand(parentIndex);
			selectionMode->select(childIndex, QItemSelectionModel::Select | QItemSelectionModel::Clear);
			ui->treeView->scrollTo(childIndex);
			break;
		}
	}
}

void QtFramedebugger::on_prevKickbutton_clicked()
{
	auto selectionMode = ui->treeView->selectionModel();
	auto packetTreeModel = ui->treeView->model();
	if(!packetTreeModel || !selectionMode)
	{
		return;
	}

	unsigned int selectedItemIndex = 0;
	auto cmdindex = GetCurrentCmdIndex();
	if(cmdindex != -1)
	{
		selectedItemIndex = cmdindex;
	}

	const auto& drawingKicks = m_frameDump.GetDrawingKicks();
	auto prevKickIndexIterator = std::prev(drawingKicks.lower_bound(selectedItemIndex));
	if(prevKickIndexIterator == std::end(drawingKicks))
	{
		//Nothing to do here
		return;
	}
	auto prevCmdIndex = prevKickIndexIterator->first;
	auto kicks = static_cast<PacketTreeModel*>(packetTreeModel)->GetDrawKickIndexes();
	for(const auto& indexes : kicks)
	{
		if(indexes.cmdIndex == prevCmdIndex)
		{
			auto parentIndex = packetTreeModel->index(indexes.parentIndex, 0);
			auto childIndex = packetTreeModel->index(indexes.childIndex, 0, parentIndex);
			ui->treeView->expand(parentIndex);
			selectionMode->select(childIndex, QItemSelectionModel::Select | QItemSelectionModel::Clear);
			ui->treeView->scrollTo(childIndex);
			break;
		}
	}
}

void QtFramedebugger::selectionChanged()
{
	auto indexes = ui->treeView->selectionModel()->selectedIndexes();
	if(!indexes.empty())
	{
		auto index = indexes.first();
		if(index.isValid())
		{
			auto cmdindex = static_cast<GsPacketData*>(index.internalPointer())->GetCmdIndex();
			UpdateDisplay(cmdindex);
		}
	}
}

void QtFramedebugger::on_tabWidget_currentChanged(int index)
{
	UpdateCurrentTab();
}

void QtFramedebugger::UpdateCurrentTab()
{
	switch(ui->tabWidget->currentIndex())
	{
	case 0:
	{
		m_gsContextView0->UpdateState(m_gs.get(), &m_currentMetadata, &m_currentDrawingKick);
		std::string result = CGsStateUtils::GetContextState(m_gs.get(), 0);
		ui->contextState0->setText(result.c_str());
	}
	break;
	case 1:
	{
		m_gsContextView1->UpdateState(m_gs.get(), &m_currentMetadata, &m_currentDrawingKick);
		std::string result = CGsStateUtils::GetContextState(m_gs.get(), 1);
		ui->contextState1->setText(result.c_str());
	}
	break;
	case 2:
	{
		std::string result = CGsStateUtils::GetInputState(m_gs.get());
		ui->inputStateTextEdit->setText(result.c_str());
	}
	break;
	case 3:
		m_vu1ProgramView->UpdateState(m_gs.get(), &m_currentMetadata, &m_currentDrawingKick);
		break;
	}
}

int QtFramedebugger::GetCurrentCmdIndex()
{
	if(ui->treeView->selectionModel())
	{
		auto indexes = ui->treeView->selectionModel()->selectedIndexes();
		if(!indexes.empty())
		{
			auto index = indexes.first();
			if(index.isValid())
			{
				auto cmdindex = static_cast<GsPacketData*>(index.internalPointer())->GetCmdIndex();
				return cmdindex;
			}
		}
	}
	return -1;
}

void QtFramedebugger::on_context0Source_currentIndexChanged(int index)
{
	m_gsContextView0->SetSelection(index);
	m_gsContextView0->UpdateState(m_gs.get(), &m_currentMetadata, &m_currentDrawingKick);
}

void QtFramedebugger::on_context0Buffer_currentIndexChanged(int index)
{
	m_gsContextView0->UpdateState(m_gs.get(), &m_currentMetadata, &m_currentDrawingKick);
}

void QtFramedebugger::on_context1Source_currentIndexChanged(int index)
{
	m_gsContextView1->SetSelection(index);
	m_gsContextView1->UpdateState(m_gs.get(), &m_currentMetadata, &m_currentDrawingKick);
}

void QtFramedebugger::on_context1Buffer_currentIndexChanged(int index)
{
	m_gsContextView1->UpdateState(m_gs.get(), &m_currentMetadata, &m_currentDrawingKick);
}

void QtFramedebugger::on_actionLoad_Dump_triggered()
{
	QFileDialog dialog(this);
	dialog.setFileMode(QFileDialog::ExistingFile);
	dialog.setNameFilter(tr("Play! Frame Dumps (*.dmp.zip);;All files (*.*)"));
	if(dialog.exec())
	{
		auto filePath = dialog.selectedFiles().first();
		LoadFrameDump(filePath.toStdString());
	}
}

void QtFramedebugger::on_actionAlpha_Test_Enabled_triggered(bool value)
{
	m_gs->SetAlphaTestingEnabled(value);
	ui->actionAlpha_Test_Enabled->setChecked(value);
	Redraw();
}

void QtFramedebugger::on_actionDepth_Test_Enabled_triggered(bool value)
{
	m_gs->SetDepthTestingEnabled(value);
	ui->actionDepth_Test_Enabled->setChecked(value);
	Redraw();
}

void QtFramedebugger::on_actionAlpha_Blend_Enabled_triggered(bool value)
{
	m_gs->SetAlphaBlendingEnabled(value);
	ui->actionAlpha_Blend_Enabled->setChecked(value);
	Redraw();
}

void QtFramedebugger::on_actionRaw_triggered(bool value)
{
	ui->actionRaw->setChecked(value);
	SetFbDisplayMode(CGsContextView::FB_DISPLAY_MODE_RAW);
	Redraw();
}

void QtFramedebugger::on_action640x448_Non_Interlaced_triggered(bool value)
{
	ui->action640x448_Non_Interlaced->setChecked(value);
	SetFbDisplayMode(CGsContextView::FB_DISPLAY_MODE_448P);
	Redraw();
}

void QtFramedebugger::on_action640x448_Interlaced_triggered(bool value)
{
	ui->action640x448_Interlaced->setChecked(value);
	SetFbDisplayMode(CGsContextView::FB_DISPLAY_MODE_448I);
	Redraw();
}

void QtFramedebugger::Redraw()
{
	uint32 selectedItemIndex = GetCurrentCmdIndex();
	if(selectedItemIndex != -1)
	{
		UpdateDisplay(selectedItemIndex);
	}
}

void QtFramedebugger::on_actionStep_VU1_triggered()
{
	StepVu1();
}
