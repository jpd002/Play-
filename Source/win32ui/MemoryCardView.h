#ifndef _MEMORYCARDVIEW_H_
#define _MEMORYCARDVIEW_H_

#include <boost/thread.hpp>
#include <boost/ptr_container/ptr_map.hpp>
#include "win32/Window.h"
#include "win32/ClientDeviceContext.h"
#include "MemoryCard.h"
#include "IconView.h"
#include "EventEx.h"
#include "../ThreadMsg.h"

class CMemoryCardView : public Framework::CWindow
{
public:
										CMemoryCardView(HWND, RECT*);
										~CMemoryCardView();

	void								SetMemoryCard(CMemoryCard*);

	Framework::CEventEx<const CSave*>	m_OnSelectionChange;

protected:
	long								OnPaint();
	long								OnVScroll(unsigned int, unsigned int);
	long								OnLeftButtonDown(int, int);
	long								OnMouseWheel(short);
	long								OnSize(unsigned int, unsigned int, unsigned int);
	long								OnKeyDown(unsigned int);

private:
	struct CViewState
	{
	public:
		int								GetCanvasSize(unsigned int);
		void							Reset(RECT*);
		void							EnsureItemFullyVisible(unsigned int);

		unsigned int					m_nItemWidth;
		unsigned int					m_nItemHeight;
		unsigned int					m_nSelection;
		int								m_nScrollPosition;
		RECT							m_ClientRect;
	};

	class CRender
	{
	public:
												CRender(HWND, const CViewState*);
												~CRender();

		void									Animate();
		void									DrawScene();
		void									SetMemoryCard(const CMemoryCard*);

	private:
		typedef boost::ptr_map<unsigned int, CIconView> IconList;

		Framework::Win32::CClientDeviceContext	m_DeviceContext;
		static PIXELFORMATDESCRIPTOR			m_PFD;
		HGLRC									m_hRC;
		IconList								m_Icons;
		const CViewState*						m_pViewState;
		const CMemoryCard*						m_pMemoryCard;
	};

	enum THREADMSG
	{
		THREAD_END,
		THREAD_SETMEMORYCARD,
	};

	void								ThreadProc();
	void								UpdateScroll();
	void								UpdateScrollPosition();
	void								UpdateGeometry();
	void								SetSelection(unsigned int);
	void								EnsureItemFullyVisible(unsigned int);

	CThreadMsg							m_MailSlot;
	CViewState							m_ViewState;
	boost::thread*						m_pThread;

	unsigned int						m_nItemCount;

	HDC									m_hDC;
	HGLRC								m_hRC;
	
	CMemoryCard*						m_pMemoryCard;
};

#endif
