#pragma once

#include <QMainWindow>

#include "gs/GSHandler.h"
#include "GsContextView.h"
#include "FrameDump.h"
#include "Vu1Vm.h"
#include "Vu1ProgramView.h"

namespace Ui
{
	class QtFramedebugger;
}

class QOffscreenSurface;

class QtFramedebugger : public QMainWindow
{
	Q_OBJECT

public:
	explicit QtFramedebugger();
	~QtFramedebugger();

protected:
	void showEvent(QShowEvent*) Q_DECL_OVERRIDE;

private slots:
	void on_action640x448_Interlaced_triggered(bool);
	void on_action640x448_Non_Interlaced_triggered(bool);
	void on_actionRaw_triggered(bool);
	void on_actionAlpha_Blend_Enabled_triggered(bool);
	void on_actionDepth_Test_Enabled_triggered(bool);
	void on_actionAlpha_Test_Enabled_triggered(bool);
	void on_actionGsHandlerOpenGL_triggered(bool);
	void on_actionGsHandlerVulkan_triggered(bool);
	void on_actionLoad_Dump_triggered();
	void on_actionStep_VU1_triggered();
	void on_context0Buffer_currentIndexChanged(int index);
	void on_context0Source_currentIndexChanged(int index);

	void on_context1Buffer_currentIndexChanged(int index);
	void on_context1Source_currentIndexChanged(int index);

	void on_tabWidget_currentChanged(int index);

	void on_prevKickbutton_clicked();
	void on_nextKickButton_clicked();

private:
	enum GS_HANDLERS
	{
		OPENGL,
#if HAS_GSH_VULKAN
		VULKAN,
#endif
		MAX_HANDLER
	};

	void selectionChanged();
	void ShowContextMenu(const QPoint&);

	void UpdateDisplay(int32);
	void UpdateCurrentTab();
	int GetCurrentCmdIndex();

	void LoadFrameDump(std::string);

	void CreateGsHandler();
	void ReleaseGsHandler();
	void SetFbDisplayMode(CGsContextView::FB_DISPLAY_MODE);
	void StepVu1();
	void Redraw();

	Ui::QtFramedebugger* ui;
	QOffscreenSurface* m_offscreenSurface = nullptr;

	std::unique_ptr<CGSHandler> m_gs;
	CGsPacketMetadata m_currentMetadata;
	DRAWINGKICK_INFO m_currentDrawingKick;
	CGsContextView::FB_DISPLAY_MODE m_fbDisplayMode = CGsContextView::FB_DISPLAY_MODE_RAW;
	CFrameDump m_frameDump;
	CVu1Vm m_vu1vm;

	std::unique_ptr<CGsContextView> m_gsContextView0;
	std::unique_ptr<CGsContextView> m_gsContextView1;
	std::unique_ptr<CVu1ProgramView> m_vu1ProgramView;
};
