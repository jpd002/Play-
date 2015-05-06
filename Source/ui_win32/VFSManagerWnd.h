#ifndef _VFSMANAGERWND_H_
#define _VFSMANAGERWND_H_

#include <string>
#include <map>
#include "win32/ModalWindow.h"
#include "win32/Button.h"
#include "win32/ListView.h"
#include "layout/LayoutObject.h"

class CVFSManagerWnd : public Framework::Win32::CModalWindow
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

		const char*                 m_sName;
		const char*                 m_sPreference;
        std::string                 m_sValue;
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
        std::string                 m_sImagePath;
        std::string                 m_sDevicePath;
		unsigned int                m_nBindingType;
	};

    typedef std::map<unsigned int, CDevice*> DeviceList;

    void							RefreshLayout();
	void							CreateListColumns();
	void							UpdateList();
	void							Save();

    Framework::LayoutObjectPtr      m_pLayout;
	Framework::Win32::CButton*      m_pOk;
	Framework::Win32::CButton*      m_pCancel;
	Framework::Win32::CListView*    m_pList;

	DeviceList                      m_devices;
};

#endif
