#pragma once

#include <memory>
#include <set>
#include <vector>
#include "win32/Window.h"
#include "win32/TreeView.h"
#include "win32/Button.h"
#include "win32/GdiObj.h"
#include "../../gs/GSHandler.h"

class CGsPacketListView : public Framework::Win32::CWindow
{
public:
	enum
	{
		NOTIFICATION_SELCHANGED = 0xA000,
	};

	struct SELCHANGED_INFO : public NMHDR
	{
		uint32 selectedCmdIndex;
	};

													CGsPacketListView(HWND, const RECT&);
	virtual											~CGsPacketListView();

	void											SetFrameDump(CFrameDump*);
	uint32											GetSelectedItemIndex() const;

protected:
	long											OnSize(unsigned int, unsigned int, unsigned int) override;
	long											OnCommand(unsigned short, unsigned short, HWND) override;
	LRESULT											OnNotify(WPARAM, NMHDR*) override;
	long											OnCopy() override;

private:
	struct PACKETINFO
	{
		HTREEITEM treeViewItem = nullptr;
		uint32    cmdIndexStart = 0;
	};

	struct WRITEINFO
	{
		HTREEITEM                 treeViewItem = nullptr;
		CGSHandler::RegisterWrite registerWrite;
	};

	uint32											GetItemIndexFromTreeViewItem(TVITEM*) const;

	long											OnPacketsTreeViewCustomDraw(NMTVCUSTOMDRAW*);
	void											OnPacketsTreeViewItemExpanding(NMTREEVIEW*);
	void											OnPacketsTreeViewSelChanged(NMTREEVIEW*);
	void											OnPacketsTreeViewKeyDown(const NMTVKEYDOWN*);

	void											GoToWrite(uint32);
	void											OnPrevDrawKick();
	void											OnNextDrawKick();

	std::unique_ptr<Framework::Win32::CTreeView>	m_packetsTreeView;
	Framework::Win32::CFont							m_drawCallItemFont;

	std::unique_ptr<Framework::Win32::CButton>		m_prevDrawKickButton;
	std::unique_ptr<Framework::Win32::CButton>		m_nextDrawKickButton;

	CFrameDump*										m_frameDump = nullptr;
	std::vector<PACKETINFO>							m_packetInfos;
	std::vector<WRITEINFO>							m_writeInfos;
};
