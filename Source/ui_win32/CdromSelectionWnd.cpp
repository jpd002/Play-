#include "CdromSelectionWnd.h"
#include "layout/LayoutStretch.h"
#include "layout/LayoutEngine.h"
#include "win32/LayoutWindow.h"
#include "win32/FileDialog.h"
#include "../ISO9660/ISO9660.h"
#include "string_cast.h"
#include "StdStream.h"
#include "PtrMacro.h"
#include "FileFilters.h"
#include <winioctl.h>

#define CLSNAME		_T("CdromSelectionWnd")
#define WNDSTYLE	(WS_CAPTION | WS_POPUP | WS_CLIPSIBLINGS | WS_CLIPCHILDREN | WS_SYSMENU)
#define WNDSTYLEEX	(WS_EX_DLGMODALFRAME)

using namespace Framework;
using namespace std;

const char* GetVendorId(const _STORAGE_DEVICE_DESCRIPTOR* descriptor)
{
	return descriptor->VendorIdOffset != 0 ? reinterpret_cast<const char*>(descriptor) + descriptor->VendorIdOffset : NULL;
}

const char* GetProductId(const _STORAGE_DEVICE_DESCRIPTOR* descriptor)
{
	return descriptor->ProductIdOffset != 0 ? reinterpret_cast<const char*>(descriptor) + descriptor->ProductIdOffset : NULL;
}

CCdromSelectionWnd::CCdromSelectionWnd(HWND hParent, const TCHAR* sTitle, CDROMBINDING* pInitBinding) :
CModalWindow(hParent)
{
	if(pInitBinding != NULL)
	{
		m_nType				= pInitBinding->nType;
		m_sImagePath		= pInitBinding->sImagePath;
		m_nPhysicalDevice	= pInitBinding->nPhysicalDevice;
	}
	else
	{
		m_nType = BINDING_IMAGE;
	}

	m_nConfirmed = false;

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
		RegisterClassEx(&w);
	}

	Create(WNDSTYLEEX, CLSNAME, sTitle, WNDSTYLE, Framework::Win32::CRect(0, 0, 300, 170), hParent, NULL);
	SetClassPtr();

	m_pOk			= new Win32::CButton(_T("OK"), m_hWnd, Framework::Win32::CRect(0, 0, 1, 1));
	m_pCancel		= new Win32::CButton(_T("Cancel"), m_hWnd, Framework::Win32::CRect(0, 0, 1, 1));

	m_pImageRadio	= new Win32::CButton(_T("ISO9660 Disk Image"), m_hWnd, Framework::Win32::CRect(0, 0, 1, 1), BS_RADIOBUTTON);
	m_pDeviceRadio	= new Win32::CButton(_T("Physical CD/DVD Reader Device"), m_hWnd, Framework::Win32::CRect(0, 0, 1, 1), BS_RADIOBUTTON);

	m_pImageEdit	= new Win32::CEdit(m_hWnd, Framework::Win32::CRect(0, 0, 1, 1), _T(""), ES_READONLY);
	m_pImageBrowse	= new Win32::CButton(_T("..."), m_hWnd, Framework::Win32::CRect(0, 0, 1, 1));
	m_pDeviceCombo	= new Win32::CComboBox(m_hWnd, Framework::Win32::CRect(0, 0, 1, 1), CBS_DROPDOWNLIST | WS_VSCROLL);

	PopulateDeviceList();

	m_pLayout =
		VerticalLayoutContainer(
			LayoutExpression(Win32::CLayoutWindow::CreateTextBoxBehavior(100, 15, m_pImageRadio)) +
			HorizontalLayoutContainer(
				LayoutExpression(Win32::CLayoutWindow::CreateTextBoxBehavior(100, 21, m_pImageEdit)) +
				LayoutExpression(Win32::CLayoutWindow::CreateButtonBehavior(20, 21, m_pImageBrowse))
			) +
			LayoutExpression(Win32::CLayoutWindow::CreateTextBoxBehavior(100, 15, m_pDeviceRadio)) +
			LayoutExpression(Win32::CLayoutWindow::CreateTextBoxBehavior(100, 20, m_pDeviceCombo)) +
			LayoutExpression(CLayoutStretch::Create()) +
			HorizontalLayoutContainer(
				LayoutExpression(CLayoutStretch::Create()) +
				LayoutExpression(Win32::CLayoutWindow::CreateButtonBehavior(100, 23, m_pOk)) +
				LayoutExpression(Win32::CLayoutWindow::CreateButtonBehavior(100, 23, m_pCancel))
			)
		);

	UpdateControls();

	RefreshLayout();
	m_pDeviceCombo->FixHeight(100);
}

CCdromSelectionWnd::~CCdromSelectionWnd()
{

}

bool CCdromSelectionWnd::WasConfirmed()
{
	return m_nConfirmed;
}

void CCdromSelectionWnd::GetBindingInfo(CDROMBINDING* pBinding)
{
	if(pBinding == NULL) return;

	pBinding->nType				= m_nType;
	pBinding->sImagePath		= m_sImagePath.c_str();
	pBinding->nPhysicalDevice	= m_nPhysicalDevice;
}

long CCdromSelectionWnd::OnCommand(unsigned short nID, unsigned short nCmd, HWND hFrom)
{
	if(hFrom == m_pOk->m_hWnd)
	{
		if(m_nType == BINDING_IMAGE)
		{
			if(m_sImagePath.length() == 0)
			{
				MessageBox(m_hWnd, _T("Please select a disk image to bind with this device."), NULL, 16);
				return FALSE;
			}
		}

		m_nConfirmed = true;
		Destroy();
		return FALSE;
	}
	if(hFrom == m_pCancel->m_hWnd)
	{
		Destroy();
		return FALSE;
	}
	if(hFrom == m_pImageRadio->m_hWnd)
	{
		m_nType = BINDING_IMAGE;
		UpdateControls();
	}
	if(hFrom == m_pImageBrowse->m_hWnd)
	{
		SelectImage();
		UpdateControls();
	}
	if(hFrom == m_pDeviceRadio->m_hWnd)
	{
		m_nType = BINDING_PHYSICAL;
		m_nPhysicalDevice = m_pDeviceCombo->GetItemData(m_pDeviceCombo->GetSelection());
		UpdateControls();
	}
	if(hFrom == m_pDeviceCombo->m_hWnd)
	{
		if(nCmd == CBN_SELCHANGE)
		{
			m_nPhysicalDevice = m_pDeviceCombo->GetItemData(m_pDeviceCombo->GetSelection());
			UpdateControls();
		}
	}
	return TRUE;
}

void CCdromSelectionWnd::UpdateControls()
{
	int nDeviceIndex;

	m_pImageRadio->SetCheck(m_nType == BINDING_IMAGE);
	m_pImageEdit->Enable(m_nType == BINDING_IMAGE);
	m_pImageBrowse->Enable(m_nType == BINDING_IMAGE);
	m_pImageEdit->SetTextA(m_sImagePath.c_str());

	m_pDeviceRadio->SetCheck(m_nType == BINDING_PHYSICAL);
	m_pDeviceCombo->Enable(m_nType == BINDING_PHYSICAL);

	nDeviceIndex = m_pDeviceCombo->FindItemData(m_nPhysicalDevice);
	m_pDeviceCombo->SetSelection(nDeviceIndex != -1 ? nDeviceIndex : 0);
}

void CCdromSelectionWnd::RefreshLayout()
{
	RECT rc = GetClientRect();

	SetRect(&rc, rc.left + 10, rc.top + 10, rc.right - 10, rc.bottom - 10);

	m_pLayout->SetRect(rc.left, rc.top, rc.right, rc.bottom);
	m_pLayout->RefreshGeometry();

	Redraw();
}

void CCdromSelectionWnd::PopulateDeviceList()
{
	uint32 nDrives, i;
	TCHAR sDeviceRoot[32];
	TCHAR sDevicePath[32];
	HANDLE hDevice;
	STORAGE_PROPERTY_QUERY Query;
	STORAGE_DEVICE_DESCRIPTOR* pDesc;
	uint8 nBuffer[512];
	DWORD nRetSize;
	TCHAR sVendorId[256];
	TCHAR sProductId[256];
	TCHAR sDisplay[256];
	unsigned int nIndex;

	nDrives = GetLogicalDrives();
	for(i = 0; i < 32; i++)
	{

		if((nDrives & (1 << i)) == 0) continue;
		_sntprintf(sDeviceRoot, countof(sDeviceRoot), _T("%c:\\"), _T('A') + i);

		if(GetDriveType(sDeviceRoot) != DRIVE_CDROM) continue;

		_sntprintf(sDevicePath, countof(sDevicePath), _T("\\\\.\\%c:"), _T('A') + i);

		_tcscpy(sVendorId, _T(""));
		_tcscpy(sProductId, _T("(Unknown device)"));

		hDevice = CreateFile(sDevicePath, GENERIC_READ, FILE_SHARE_READ, NULL, OPEN_EXISTING, FILE_FLAG_NO_BUFFERING, NULL);
		if(hDevice != INVALID_HANDLE_VALUE)
		{
			memset(&Query, 0, sizeof(STORAGE_PROPERTY_QUERY));
			Query.PropertyId	= StorageDeviceProperty;
			Query.QueryType		= PropertyStandardQuery;

			if(DeviceIoControl(hDevice, IOCTL_STORAGE_QUERY_PROPERTY, &Query, sizeof(STORAGE_PROPERTY_QUERY), &nBuffer, 512, &nRetSize, NULL) != 0)
			{
				pDesc = (STORAGE_DEVICE_DESCRIPTOR*)nBuffer;

				_tcsncpy(sVendorId, GetVendorId(pDesc) != NULL ? string_cast<tstring>(GetVendorId(pDesc)).c_str() : _T(""), 256);
				_tcsncpy(sProductId, GetProductId(pDesc) != NULL ? string_cast<tstring>(GetProductId(pDesc)).c_str() : _T(""), 256);

				if(GetVendorId(pDesc) != NULL)
				{
					_tcscat(sVendorId, _T(" "));
				}
			}

			CloseHandle(hDevice);
		}

		_sntprintf(sDisplay, countof(sDisplay), _T("%s - %s%s"),  sDeviceRoot, sVendorId, sProductId);

		nIndex = m_pDeviceCombo->AddString(sDisplay);
		m_pDeviceCombo->SetItemData(nIndex, i);
	}
}

void CCdromSelectionWnd::SelectImage()
{
	Win32::CFileDialog Dialog;

	Dialog.m_OFN.lpstrFilter = DISKIMAGE_FILTER;

	if(Dialog.SummonOpen(m_hWnd) != IDOK)
	{
		return;
	}

    string sPath(string_cast<string>(Dialog.GetPath()));
/*
	CStdStream* pStream;
	try
	{
        pStream = new CStdStream(fopen(sPath.c_str(), "rb"));
	}
	catch(...)
	{
		CWindow::MessageBoxFormat(m_hWnd, 16, NULL, _T("Couldn't open file '%s' for reading."), Dialog.m_sFile);
		return;
	}

	CISO9660* pISO;
    try
	{
		pISO = new CISO9660(pStream);
	}
	catch(...)
	{
		CWindow::MessageBoxFormat(m_hWnd, 16, NULL, _T("File '%s' is an unrecognized ISO9660 disk image."), Dialog.m_sFile);
		delete pStream;
		return;
	}

	delete pISO;
*/
	m_sImagePath = sPath.c_str();
}
