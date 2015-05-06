#include <shlobj.h>
#include <stdio.h>
#include "VFSManagerWnd.h"
#include "layout/LayoutEngine.h"
#include "layout/LayoutStretch.h"
#include "layout/HorizontalLayout.h"
#include "win32/LayoutWindow.h"
#include "win32/Static.h"
#include "PtrMacro.h"
#include "../AppConfig.h"
#include "../PS2VM_Preferences.h"
#include "CdromSelectionWnd.h"
#include "string_cast.h"

#define CLSNAME			_T("VFSManagerWnd")
#define WNDSTYLE		(WS_CAPTION | WS_POPUP | WS_CLIPSIBLINGS | WS_CLIPCHILDREN | WS_SYSMENU)
#define WNDSTYLEEX		(WS_EX_DLGMODALFRAME)
#define SCALE(x)		MulDiv(x, ydpi, 96)

using namespace Framework;
using namespace std;

CVFSManagerWnd::CVFSManagerWnd(HWND hParent) :
CModalWindow(hParent)
{
	if(!DoesWindowClassExist(CLSNAME))
	{
		WNDCLASSEX w;
		memset(&w, 0, sizeof(WNDCLASSEX));
		w.cbSize		= sizeof(WNDCLASSEX);
		w.lpfnWndProc	= CWindow::WndProc;
		w.lpszClassName	= CLSNAME;
		w.hbrBackground	= (HBRUSH)GetSysColorBrush(COLOR_BTNFACE);
		w.hInstance		= GetModuleHandle(NULL);
		w.hCursor		= LoadCursor(NULL, IDC_ARROW);
		w.style			= CS_HREDRAW | CS_VREDRAW;
		RegisterClassEx(&w);
	}

	int ydpi = GetDeviceCaps(GetDC(NULL), LOGPIXELSY);

	Create(WNDSTYLEEX, CLSNAME, _T("Virtual File System Manager"), WNDSTYLE, Framework::Win32::CRect(0, 0, SCALE(400), SCALE(250)), hParent, NULL);
	SetClassPtr();

	m_pList = new Win32::CListView(m_hWnd, Framework::Win32::CRect(0, 0, 1, 1), LVS_REPORT | LVS_SORTASCENDING);
	m_pList->SetExtendedListViewStyle(m_pList->GetExtendedListViewStyle() | LVS_EX_FULLROWSELECT);

	m_pOk		= new Win32::CButton(_T("OK"), m_hWnd, Framework::Win32::CRect(0, 0, 1, 1));
	m_pCancel	= new Win32::CButton(_T("Cancel"), m_hWnd, Framework::Win32::CRect(0, 0, 1, 1));

	m_pLayout =
		VerticalLayoutContainer(
			LayoutExpression(Win32::CLayoutWindow::CreateCustomBehavior(1, 1, 1, 1, m_pList)) +
			LayoutExpression(Win32::CLayoutWindow::CreateTextBoxBehavior(SCALE(100), SCALE(50), new Win32::CStatic(m_hWnd, _T("Warning: Changing file system bindings while a program is running might be dangerous. Changes to 'cdrom0' bindings will take effect next time you load an executable."), SS_LEFT))) +
			HorizontalLayoutContainer(
				LayoutExpression(CLayoutStretch::Create()) +
				LayoutExpression(Win32::CLayoutWindow::CreateButtonBehavior(SCALE(100), SCALE(23), m_pOk)) +
				LayoutExpression(Win32::CLayoutWindow::CreateButtonBehavior(SCALE(100), SCALE(23), m_pCancel))
			)
		);

	RefreshLayout();

	m_devices[0] = new CDirectoryDevice("mc0",  "ps2.mc0.directory");
	m_devices[1] = new CDirectoryDevice("mc1",  "ps2.mc1.directory");
	m_devices[2] = new CDirectoryDevice("host", "ps2.host.directory");
	m_devices[3] = new CCdrom0Device();

	CreateListColumns();
	UpdateList();
}

CVFSManagerWnd::~CVFSManagerWnd()
{
    for(DeviceList::const_iterator deviceIterator(m_devices.begin());
        m_devices.end() != deviceIterator; deviceIterator++)
    {
        delete deviceIterator->second;
    }
}

long CVFSManagerWnd::OnCommand(unsigned short nID, unsigned short nCmd, HWND hSender)
{
	if(hSender == m_pOk->m_hWnd)
	{
		Save();
		Destroy();
		return TRUE;
	}
	else if(hSender == m_pCancel->m_hWnd)
	{
		Destroy();
		return TRUE;
	}
	return FALSE;
}

long CVFSManagerWnd::OnNotify(WPARAM wParam, NMHDR* pHDR)
{
    if(pHDR->hwndFrom == m_pList->m_hWnd)
    {
        switch(pHDR->code)
        {
        case NM_DBLCLK:
            int nSel = m_pList->GetSelection();
            if(nSel != -1)
            {
                DeviceList::const_iterator deviceIterator(m_devices.find(m_pList->GetItemData(nSel)));
                if(deviceIterator != m_devices.end())
                {
                    CDevice* pDevice(deviceIterator->second);
                    if(pDevice->RequestModification(m_hWnd))
                    {
                        UpdateList();
                    }
                }
            }
            break;
        }
    }

    return FALSE;
}

void CVFSManagerWnd::RefreshLayout()
{
	RECT rc = GetClientRect();

	SetRect(&rc, rc.left + 10, rc.top + 10, rc.right - 10, rc.bottom - 10);

	m_pLayout->SetRect(rc.left, rc.top, rc.right, rc.bottom);
	m_pLayout->RefreshGeometry();

	Redraw();
}

void CVFSManagerWnd::CreateListColumns()
{
	LVCOLUMN col;
	RECT rc = m_pList->GetClientRect();

	memset(&col, 0, sizeof(LVCOLUMN));
	col.pszText = _T("Device");
	col.mask	= LVCF_TEXT | LVCF_WIDTH;
	col.cx		= rc.right / 4;
	m_pList->InsertColumn(0, col);

	memset(&col, 0, sizeof(LVCOLUMN));
	col.pszText = _T("Binding Type");
	col.mask	= LVCF_TEXT | LVCF_WIDTH;
	col.cx		= rc.right / 4;
	m_pList->InsertColumn(1, col);

	memset(&col, 0, sizeof(LVCOLUMN));
	col.pszText = _T("Binding Value");
	col.mask	= LVCF_TEXT | LVCF_WIDTH;
	col.cx		= rc.right / 2;
	m_pList->InsertColumn(2, col);
}

void CVFSManagerWnd::UpdateList()
{
    for(DeviceList::const_iterator deviceIterator(m_devices.begin());
        m_devices.end() != deviceIterator; deviceIterator++)
    {
        CDevice* pDevice = deviceIterator->second;
        unsigned int key = deviceIterator->first;

        unsigned int index = m_pList->FindItemData(key);
        if(index == -1)
        {
            tstring sDeviceName(string_cast<tstring>(pDevice->GetDeviceName()));

            LVITEM itm;
            memset(&itm, 0, sizeof(LVITEM));
            itm.mask		= LVIF_TEXT | LVIF_PARAM;
            itm.pszText		= const_cast<TCHAR*>(sDeviceName.c_str());
            itm.lParam		= key;
            index = m_pList->InsertItem(itm);
        }

        m_pList->SetItemText(index, 1, string_cast<tstring>(pDevice->GetBindingType()).c_str());
        m_pList->SetItemText(index, 2, string_cast<tstring>(pDevice->GetBinding()).c_str());
    }
}

void CVFSManagerWnd::Save()
{
    for(DeviceList::const_iterator deviceIterator(m_devices.begin());
        m_devices.end() != deviceIterator; deviceIterator++)
    {
        CDevice* pDevice = deviceIterator->second;
        pDevice->Save();
    }
}

///////////////////////////////////////////
//CDevice Implementation
///////////////////////////////////////////

CVFSManagerWnd::CDevice::~CDevice()
{

}

///////////////////////////////////////////
//CDirectoryDevice Implementation
///////////////////////////////////////////

CVFSManagerWnd::CDirectoryDevice::CDirectoryDevice(const char* sName, const char* sPreference)
{
    m_sName         = sName;
    m_sPreference   = sPreference;
    m_sValue        = CAppConfig::GetInstance().GetPreferenceString(m_sPreference);
}

CVFSManagerWnd::CDirectoryDevice::~CDirectoryDevice()
{

}

const char* CVFSManagerWnd::CDirectoryDevice::GetDeviceName()
{
    return m_sName;
}

const char* CVFSManagerWnd::CDirectoryDevice::GetBindingType()
{
    return "Directory";
}

const char* CVFSManagerWnd::CDirectoryDevice::GetBinding()
{
    return m_sValue.c_str();
}

bool CVFSManagerWnd::CDirectoryDevice::RequestModification(HWND hParent)
{
	BROWSEINFO bi;
	LPITEMIDLIST item;
	TCHAR sPath[MAX_PATH];

	memset(&bi, 0, sizeof(BROWSEINFO));
	bi.hwndOwner	= hParent;
	bi.lpszTitle	= _T("Select new folder for device");
	bi.ulFlags		= BIF_RETURNONLYFSDIRS;
	bi.lpfn			= BrowseCallback;
	bi.lParam		= (LPARAM)this;

	item = SHBrowseForFolder(&bi);
	
	if(item == NULL)
	{
		return false;
	}

	if(SHGetPathFromIDList(item, sPath) == 0)
	{
		MessageBox(hParent, _T("Invalid directory."), NULL, 16);
		CoTaskMemFree(item);
		return false;
	}

	CoTaskMemFree(item);

	m_sValue = string_cast<string>(sPath).c_str();

	return true;
}

void CVFSManagerWnd::CDirectoryDevice::Save()
{
	CAppConfig::GetInstance().SetPreferenceString(m_sPreference, m_sValue.c_str());
}

int CVFSManagerWnd::CDirectoryDevice::BrowseCallback(HWND hFrom, unsigned int nMsg, LPARAM lParam, LPARAM pData)
{
	CDirectoryDevice* pDevice = reinterpret_cast<CDirectoryDevice*>(pData);
	switch(nMsg)
	{
	case BFFM_INITIALIZED:
        tstring sPath(string_cast<tstring>(pDevice->m_sValue.c_str()));
		SendMessage(hFrom, BFFM_SETSELECTION, TRUE, reinterpret_cast<LPARAM>(sPath.c_str()));
		break;
	}
	return 0;
}

///////////////////////////////////////////
//CCdrom0Device Implementation
///////////////////////////////////////////

CVFSManagerWnd::CCdrom0Device::CCdrom0Device()
{
	char sDevicePath[32];

	const char* sPath = CAppConfig::GetInstance().GetPreferenceString(PS2VM_CDROM0PATH);
	
	//Detect the binding type from the path format
	if(!strcmp(sPath, ""))
	{
		m_nBindingType	= CCdromSelectionWnd::BINDING_IMAGE;
		m_sImagePath	= "";
	}
	else if(strlen(sPath) == 6 && !strncmp(sPath, "\\\\.\\", 4))
	{
		sprintf(sDevicePath, "%c:\\", toupper(sPath[4]));

		m_nBindingType	= CCdromSelectionWnd::BINDING_PHYSICAL;
		m_sDevicePath	= sDevicePath;
	}
	else
	{
		m_nBindingType	= CCdromSelectionWnd::BINDING_IMAGE;
		m_sImagePath	= sPath;
	}
}

CVFSManagerWnd::CCdrom0Device::~CCdrom0Device()
{

}

const char* CVFSManagerWnd::CCdrom0Device::GetDeviceName()
{
	return "cdrom0";
}

const char* CVFSManagerWnd::CCdrom0Device::GetBindingType()
{
	if(m_nBindingType == CCdromSelectionWnd::BINDING_PHYSICAL)
	{
		return "Physical Device";
	}
	if(m_nBindingType == CCdromSelectionWnd::BINDING_IMAGE)
	{
		return "Disk Image";
	}
	return "";
}

const char* CVFSManagerWnd::CCdrom0Device::GetBinding()
{
	if(m_nBindingType == CCdromSelectionWnd::BINDING_IMAGE)
	{
		if(m_sImagePath.length() == 0)
		{
			return "(None)";
		}
		else
		{
			return m_sImagePath.c_str();
		}
	}
	if(m_nBindingType == CCdromSelectionWnd::BINDING_PHYSICAL)
	{
		return m_sDevicePath.c_str();
	}
	return "";
}

bool CVFSManagerWnd::CCdrom0Device::RequestModification(HWND hParent)
{
	CCdromSelectionWnd::CDROMBINDING Binding;
	char sDevicePath[32];

	Binding.nType				= (CCdromSelectionWnd::BINDINGTYPE)m_nBindingType;
	Binding.sImagePath			= m_sImagePath.c_str();
	Binding.nPhysicalDevice		= m_sDevicePath[0] - 'A';

	CCdromSelectionWnd SelectionWnd(hParent, _T("Modify cdrom0 Binding"), &Binding);

	SelectionWnd.DoModal();

	if(!SelectionWnd.WasConfirmed()) return false;

	SelectionWnd.GetBindingInfo(&Binding);

	sprintf(sDevicePath, "%c:\\", Binding.nPhysicalDevice + 'A');

	m_nBindingType	= Binding.nType;
	m_sImagePath	= Binding.sImagePath;
	m_sDevicePath	= sDevicePath;

	return true;
}

void CVFSManagerWnd::CCdrom0Device::Save()
{
    if(m_nBindingType == CCdromSelectionWnd::BINDING_IMAGE)
    {
        CAppConfig::GetInstance().SetPreferenceString(PS2VM_CDROM0PATH, m_sImagePath.c_str());
    }
    if(m_nBindingType == CCdromSelectionWnd::BINDING_PHYSICAL)
    {
        char sDevicePath[32];
        sprintf(sDevicePath, "\\\\.\\%c:", m_sDevicePath[0]);
        CAppConfig::GetInstance().SetPreferenceString(PS2VM_CDROM0PATH, sDevicePath);
    }
}
