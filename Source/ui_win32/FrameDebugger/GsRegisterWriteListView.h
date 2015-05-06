#pragma once

#include <memory>
#include <set>
#include <vector>
#include "win32/Window.h"
#include "win32/TreeView.h"
#include "win32/Button.h"
#include "win32/GdiObj.h"
#include "../../gs/GSHandler.h"

class CGsRegisterWriteListView : public Framework::Win32::CWindow
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

													CGsRegisterWriteListView(HWND, const RECT&);
	virtual											~CGsRegisterWriteListView();

	void											SetFrameDump(CFrameDump*);
	uint32											GetSelectedItemIndex() const;

protected:
	virtual long									OnSize(unsigned int, unsigned int, unsigned int) override;
	virtual long									OnCommand(unsigned short, unsigned short, HWND) override;
	virtual long									OnNotify(WPARAM, NMHDR*) override;

private:
	struct PACKETINFO
	{
		PACKETINFO()
			: cmdIndexStart(0)
			, treeViewItem(nullptr)
		{

		}

		uint32		cmdIndexStart;
		HTREEITEM	treeViewItem;
	};

	struct WRITEINFO
	{
		WRITEINFO()
			: treeViewItem(nullptr)
		{

		}

		HTREEITEM	treeViewItem;
	};

	uint32											GetItemIndexFromTreeViewItem(TVITEM*) const;

	long											OnPacketsTreeViewCustomDraw(NMTVCUSTOMDRAW*);
	void											OnPacketsTreeViewItemExpanding(NMTREEVIEW*);
	void											OnPacketsTreeViewSelChanged(NMTREEVIEW*);

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
