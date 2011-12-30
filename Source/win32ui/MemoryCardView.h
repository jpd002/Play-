#ifndef _MEMORYCARDVIEW_H_
#define _MEMORYCARDVIEW_H_

#include <boost/thread.hpp>
#include <boost/signal.hpp>
#include <map>
#include "win32/Window.h"
#include "win32/ClientDeviceContext.h"
#include "MemoryCard.h"
#include "IconMesh.h"
#include "../MailBox.h"

class CMemoryCardView : public Framework::Win32::CWindow
{
public:
										CMemoryCardView(HWND, RECT*);
										~CMemoryCardView();

	void								SetMemoryCard(CMemoryCard*);

    boost::signal<void (const CSave*)>  m_OnSelectionChange;

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
		typedef std::map<unsigned int, IconMeshPtr> IconList;

		void									ThreadProc();
		void									ThreadSetMemoryCard(const CMemoryCard*);

		Framework::Win32::CClientDeviceContext	m_deviceContext;
		static const PIXELFORMATDESCRIPTOR		m_PFD;
		HGLRC									m_hRC;
		IconList								m_icons;
		const CViewState*						m_viewState;
		const CMemoryCard*						m_memoryCard;

		CMailBox								m_mailBox;
		boost::thread*							m_thread;
		bool									m_threadOver;
	};

	void								UpdateScroll();
	void								UpdateScrollPosition();
	void								UpdateGeometry();
	void								SetSelection(unsigned int);
	void								EnsureItemFullyVisible(unsigned int);

	unsigned int						m_itemCount;
	CViewState							m_viewState;
	
	CMemoryCard*						m_memoryCard;
	CRender*							m_render;
};

#endif
