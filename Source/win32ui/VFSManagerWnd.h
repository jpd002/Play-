#ifndef _VFSMANAGERWND_H_
#define _VFSMANAGERWND_H_

#include "ModalWindow.h"
#include "win32/Button.h"
#include "win32/ListView.h"
#include "List.h"
#include "VerticalLayout.h"
#include "Str.h"

class CVFSManagerWnd : public CModalWindow
{
public:
									CVFSManagerWnd(HWND);
									~CVFSManagerWnd();

protected:
	long							OnCommand(unsigned short, unsigned short, HWND);
	long							OnNotify(WPARAM, NMHDR*);

private:
	class CDevice
	{
	public:
		virtual						~CDevice();
		virtual const char*			GetDeviceName() = 0;
		virtual const char*			GetBindingType() = 0;
		virtual const char*			GetBinding() = 0;
		virtual bool				RequestModification(HWND) = 0;
		virtual void				Save() = 0;
	};

	class CDirectoryDevice : public CDevice
	{
	public:
									CDirectoryDevice(const char*, const char*);
		virtual						~CDirectoryDevice();
		virtual const char*			GetDeviceName();
		virtual const char*			GetBindingType();
		virtual const char*			GetBinding();
		virtual bool				RequestModification(HWND);
		virtual void				Save();

	private:
		static int WINAPI			BrowseCallback(HWND, unsigned int, LPARAM, LPARAM);

		const char*					m_sName;
		const char*					m_sPreference;
		Framework::CStrA			m_sValue;
	};

	class CCdrom0Device : public CDevice
	{
	public:
									CCdrom0Device();
		virtual						~CCdrom0Device();
		virtual const char*			GetDeviceName();
		virtual const char*			GetBindingType();
		virtual const char*			GetBinding();
		virtual bool				RequestModification(HWND);
		virtual void				Save();

	private:
		Framework::CStrA			m_sImagePath;
		Framework::CStrA			m_sDevicePath;
		unsigned int				m_nBindingType;
	};

	void							RefreshLayout();
	void							CreateListColumns();
	void							UpdateList();
	void							Save();

	Framework::CVerticalLayout*		m_pLayout;
	Framework::Win32::CButton*		m_pOk;
	Framework::Win32::CButton*		m_pCancel;
	Framework::Win32::CListView*	m_pList;

	Framework::CList<CDevice>		m_Device;
};

#endif
