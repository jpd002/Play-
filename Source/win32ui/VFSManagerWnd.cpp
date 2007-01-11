#include <shlobj.h>
#include <stdio.h>
#include "VFSManagerWnd.h"
#include "LayoutStretch.h"
#include "HorizontalLayout.h"
#include "win32/LayoutWindow.h"
#include "win32/Static.h"
#include "PtrMacro.h"
#include "../Config.h"
#include "CdromSelectionWnd.h"

#define CLSNAME			_X("VFSManagerWnd")
#define WNDSTYLE		(WS_CAPTION | WS_POPUP | WS_CLIPSIBLINGS | WS_CLIPCHILDREN | WS_SYSMENU)
#define WNDSTYLEEX		(WS_EX_DLGMODALFRAME)
#define CDROM0PATH		"ps2.cdrom0.path"

using namespace Framework;

CVFSManagerWnd::CVFSManagerWnd(HWND hParent) :
CModalWindow(hParent)
{
	RECT rc;
	CHorizontalLayout* pSubLayout0;

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

	SetRect(&rc, 0, 0, 400, 250);

	Create(WNDSTYLEEX, CLSNAME, _X("Virtual File System Manager"), WNDSTYLE, &rc, hParent, NULL);
	SetClassPtr();

	SetRect(&rc, 0, 0, 1, 1);

	m_pList = new Win32::CListView(m_hWnd, &rc, LVS_REPORT | LVS_SORTASCENDING);
	m_pList->SetExtendedListViewStyle(m_pList->GetExtendedListViewStyle() | LVS_EX_FULLROWSELECT);

	m_pOk		= new Win32::CButton(_X("OK"), m_hWnd, &rc);
	m_pCancel	= new Win32::CButton(_X("Cancel"), m_hWnd, &rc);

	pSubLayout0 = new CHorizontalLayout;
	pSubLayout0->InsertObject(new CLayoutStretch);
	pSubLayout0->InsertObject(CLayoutWindow::CreateButtonBehavior(100, 23, m_pOk));
	pSubLayout0->InsertObject(CLayoutWindow::CreateButtonBehavior(100, 23, m_pCancel));
	pSubLayout0->SetVerticalStretch(0);

	m_pLayout = new CVerticalLayout;
	m_pLayout->InsertObject(new CLayoutWindow(1, 1, 1, 1, m_pList));
	m_pLayout->InsertObject(CLayoutWindow::CreateTextBoxBehavior(100, 40, new Win32::CStatic(m_hWnd, _X("Warning: Changing file system bindings while a program is running might be dangerous. Changes to 'cdrom0' bindings will take effect next time you load an executable."), SS_LEFT)));
	m_pLayout->InsertObject(pSubLayout0);

	RefreshLayout();

	m_Device.Insert(new CDirectoryDevice("mc0",		"ps2.mc0.directory"),	0);
	m_Device.Insert(new CDirectoryDevice("mc1",		"ps2.mc1.directory"),	1);
	m_Device.Insert(new CDirectoryDevice("host",	"ps2.host.directory"),	2);
	m_Device.Insert(new CCdrom0Device(),									3);

	CreateListColumns();
	UpdateList();
}

CVFSManagerWnd::~CVFSManagerWnd()
{
	DELETEPTR(m_pLayout);

	while(m_Device.Count() != 0)
	{
		delete m_Device.Pull();
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
	int nSel;
	CDevice* pDevice;

	if(pHDR->hwndFrom == m_pList->m_hWnd)
	{
		switch(pHDR->code)
		{
		case NM_DBLCLK:
			nSel = m_pList->GetSelection();
			if(nSel != -1)
			{
				pDevice = m_Device.Find(m_pList->GetItemData(nSel));
				if(pDevice != NULL)
				{
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
	RECT rc;

	GetClientRect(&rc);

	SetRect(&rc, rc.left + 10, rc.top + 10, rc.right - 10, rc.bottom - 10);

	m_pLayout->SetRect(rc.left, rc.top, rc.right, rc.bottom);
	m_pLayout->RefreshGeometry();

	Redraw();
}

void CVFSManagerWnd::CreateListColumns()
{
	LVCOLUMN col;
	RECT rc;

	m_pList->GetClientRect(&rc);

	memset(&col, 0, sizeof(LVCOLUMN));
	col.pszText = _X("Device");
	col.mask	= LVCF_TEXT | LVCF_WIDTH;
	col.cx		= rc.right / 4;
	m_pList->InsertColumn(0, &col);

	memset(&col, 0, sizeof(LVCOLUMN));
	col.pszText = _X("Binding Type");
	col.mask	= LVCF_TEXT | LVCF_WIDTH;
	col.cx		= rc.right / 4;
	m_pList->InsertColumn(1, &col);

	memset(&col, 0, sizeof(LVCOLUMN));
	col.pszText = _X("Binding Value");
	col.mask	= LVCF_TEXT | LVCF_WIDTH;
	col.cx		= rc.right / 2;
	m_pList->InsertColumn(2, &col);
}

void CVFSManagerWnd::UpdateList()
{
	unsigned int i;
	CList<CDevice>::ITERATOR itDevice;
	CDevice* pDevice;
	LVITEM itm;
	xchar sConvert[256];

	for(itDevice = m_Device.Begin(); itDevice.HasNext(); itDevice++)
	{
		pDevice = (*itDevice);

		xconvert(sConvert, pDevice->GetDeviceName(), 256);

		i = m_pList->FindItemData(itDevice.GetKey());
		if(i == -1)
		{
			memset(&itm, 0, sizeof(LVITEM));
			itm.mask		= LVIF_TEXT | LVIF_PARAM;
			itm.pszText		= sConvert;
			itm.lParam		= itDevice.GetKey();
			i = m_pList->InsertItem(&itm);
		}

		xconvert(sConvert, pDevice->GetBindingType(), 256);
		m_pList->SetItemText(i, 1, sConvert);

		xconvert(sConvert, pDevice->GetBinding(), 256);
		m_pList->SetItemText(i, 2, sConvert);
	}
}

void CVFSManagerWnd::Save()
{
	CList<CDevice>::ITERATOR itDevice;
	CDevice* pDevice;

	for(itDevice = m_Device.Begin(); itDevice.HasNext(); itDevice++)
	{
		pDevice = (*itDevice);
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
	m_sName			= sName;
	m_sPreference	= sPreference;
	m_sValue		= CConfig::GetInstance()->GetPreferenceString(m_sPreference);
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
	return m_sValue;
}

bool CVFSManagerWnd::CDirectoryDevice::RequestModification(HWND hParent)
{
	BROWSEINFO bi;
	LPITEMIDLIST item;
	xchar sPath[MAX_PATH];
	char sConvert[MAX_PATH];

	memset(&bi, 0, sizeof(BROWSEINFO));
	bi.hwndOwner	= hParent;
	bi.lpszTitle	= _X("Select new folder for device");
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
		MessageBox(hParent, _X("Invalid directory."), NULL, 16);
		CoTaskMemFree(item);
		return false;
	}

	CoTaskMemFree(item);

	xconvert(sConvert, sPath, MAX_PATH);
	m_sValue = sConvert;

	return true;
}

void CVFSManagerWnd::CDirectoryDevice::Save()
{
	CConfig::GetInstance()->SetPreferenceString(m_sPreference, m_sValue);
}

int CVFSManagerWnd::CDirectoryDevice::BrowseCallback(HWND hFrom, unsigned int nMsg, LPARAM lParam, LPARAM pData)
{
	xchar* sConvert;
	CDirectoryDevice* pDevice;

	pDevice = (CDirectoryDevice*)pData;

	switch(nMsg)
	{
	case BFFM_INITIALIZED:
		sConvert = new xchar[strlen(pDevice->m_sValue) + 1];
		xconvert(sConvert, (const char*)pDevice->m_sValue, strlen(pDevice->m_sValue) + 1);
		SendMessage(hFrom, BFFM_SETSELECTION, TRUE, (LPARAM)sConvert);
		delete sConvert;
		break;
	}
	return 0;
}

///////////////////////////////////////////
//CCdrom0Device Implementation
///////////////////////////////////////////

CVFSManagerWnd::CCdrom0Device::CCdrom0Device()
{
	const char* sPath;
	char sDevicePath[32];

	sPath = CConfig::GetInstance()->GetPreferenceString(CDROM0PATH);
	
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
		if(strlen(m_sImagePath) == 0)
		{
			return "(None)";
		}
		else
		{
			return m_sImagePath;
		}
	}
	if(m_nBindingType == CCdromSelectionWnd::BINDING_PHYSICAL)
	{
		return m_sDevicePath;
	}
	return "";
}

bool CVFSManagerWnd::CCdrom0Device::RequestModification(HWND hParent)
{
	CCdromSelectionWnd::CDROMBINDING Binding;
	char sDevicePath[32];

	Binding.nType				= (CCdromSelectionWnd::BINDINGTYPE)m_nBindingType;
	Binding.sImagePath			= m_sImagePath;
	Binding.nPhysicalDevice		= ((const char*)m_sDevicePath)[0] - 'A';

	CCdromSelectionWnd SelectionWnd(hParent, _X("Modify cdrom0 Binding"), &Binding);

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
	char sDevicePath[32];

	if(m_nBindingType == CCdromSelectionWnd::BINDING_IMAGE)
	{
		CConfig::GetInstance()->SetPreferenceString(CDROM0PATH, m_sImagePath);
	}
	if(m_nBindingType == CCdromSelectionWnd::BINDING_PHYSICAL)
	{
		sprintf(sDevicePath, "\\\\.\\%c:", ((const char*)m_sDevicePath)[0]);
		CConfig::GetInstance()->SetPreferenceString(CDROM0PATH, sDevicePath);
	}
}
