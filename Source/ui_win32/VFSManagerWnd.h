#pragma once

#include <string>
#include <map>
#include <memory>
#include "win32/ModalWindow.h"
#include "win32/Button.h"
#include "win32/ListView.h"
#include "layout/LayoutObject.h"

class CVFSManagerWnd : public Framework::Win32::CModalWindow
{
public:
									CVFSManagerWnd(HWND);
	virtual							~CVFSManagerWnd() = default;

protected:
	long							OnCommand(unsigned short, unsigned short, HWND) override;
	LRESULT							OnNotify(WPARAM, NMHDR*) override;

private:
	class CDevice
	{
	public:
		virtual						~CDevice() = default;
		virtual const char*			GetDeviceName() = 0;
		virtual const char*			GetBindingType() = 0;
		virtual std::tstring		GetBinding() = 0;
		virtual bool				RequestModification(HWND) = 0;
		virtual void				Save() = 0;
	};

	class CDirectoryDevice : public CDevice
	{
	public:
									CDirectoryDevice(const char*, const char*);
		virtual						~CDirectoryDevice() = default;
		const char*					GetDeviceName() override;
		const char*					GetBindingType() override;
		std::tstring				GetBinding() override;
		bool						RequestModification(HWND) override;
		void						Save() override;

	private:
		static int WINAPI			BrowseCallback(HWND, unsigned int, LPARAM, LPARAM);

		const char*					m_name;
		const char*					m_preference;
		std::tstring				m_path;
	};

	class CCdrom0Device : public CDevice
	{
	public:
									CCdrom0Device();
		virtual						~CCdrom0Device() = default;
		const char*					GetDeviceName() override;
		const char*					GetBindingType() override;
		std::tstring				GetBinding() override;
		bool						RequestModification(HWND) override;
		void						Save() override;

	private:
		std::tstring				m_imagePath;
		std::string					m_devicePath;
		unsigned int				m_bindingType;
	};

	typedef std::unique_ptr<CDevice> DevicePtr;
	typedef std::map<unsigned int, DevicePtr> DeviceList;

	void							RefreshLayout();
	void							CreateListColumns();
	void							UpdateList();
	void							Save();

	Framework::LayoutObjectPtr		m_pLayout;
	Framework::Win32::CButton*		m_pOk = nullptr;
	Framework::Win32::CButton*		m_pCancel = nullptr;
	Framework::Win32::CListView*	m_pList = nullptr;

	DeviceList						m_devices;
};
