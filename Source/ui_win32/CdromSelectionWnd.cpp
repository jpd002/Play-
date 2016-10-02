#include "CdromSelectionWnd.h"
#include "layout/LayoutStretch.h"
#include "layout/LayoutEngine.h"
#include "win32/LayoutWindow.h"
#include "win32/FileDialog.h"
#include "win32/DefaultWndClass.h"
#include "string_cast.h"
#include "FileFilters.h"
#include <winioctl.h>

#define WNDSTYLE	(WS_CAPTION | WS_POPUP | WS_CLIPSIBLINGS | WS_CLIPCHILDREN | WS_SYSMENU)
#define WNDSTYLEEX	(WS_EX_DLGMODALFRAME)

const char* GetVendorId(const _STORAGE_DEVICE_DESCRIPTOR* descriptor)
{
	return descriptor->VendorIdOffset != 0 ? reinterpret_cast<const char*>(descriptor) + descriptor->VendorIdOffset : NULL;
}

const char* GetProductId(const _STORAGE_DEVICE_DESCRIPTOR* descriptor)
{
	return descriptor->ProductIdOffset != 0 ? reinterpret_cast<const char*>(descriptor) + descriptor->ProductIdOffset : NULL;
}

CCdromSelectionWnd::CCdromSelectionWnd(HWND hParent, const TCHAR* sTitle, CDROMBINDING* pInitBinding)
: CModalWindow(hParent)
{
	if(pInitBinding != NULL)
	{
		m_nType				= pInitBinding->nType;
		m_sImagePath		= pInitBinding->sImagePath;
		m_nPhysicalDevice	= pInitBinding->nPhysicalDevice;
	}

	Create(WNDSTYLEEX, Framework::Win32::CDefaultWndClass::GetName(), sTitle, WNDSTYLE, Framework::Win32::CRect(0, 0, 300, 170), hParent, NULL);
	SetClassPtr();

	m_pOk			= new Framework::Win32::CButton(_T("OK"), m_hWnd, Framework::Win32::CRect(0, 0, 1, 1));
	m_pCancel		= new Framework::Win32::CButton(_T("Cancel"), m_hWnd, Framework::Win32::CRect(0, 0, 1, 1));

	m_pImageRadio	= new Framework::Win32::CButton(_T("ISO9660 Disk Image"), m_hWnd, Framework::Win32::CRect(0, 0, 1, 1), BS_RADIOBUTTON);
	m_pDeviceRadio	= new Framework::Win32::CButton(_T("Physical CD/DVD Reader Device"), m_hWnd, Framework::Win32::CRect(0, 0, 1, 1), BS_RADIOBUTTON);

	m_pImageEdit	= new Framework::Win32::CEdit(m_hWnd, Framework::Win32::CRect(0, 0, 1, 1), _T(""), ES_READONLY);
	m_pImageBrowse	= new Framework::Win32::CButton(_T("..."), m_hWnd, Framework::Win32::CRect(0, 0, 1, 1));
	m_pDeviceCombo	= new Framework::Win32::CComboBox(m_hWnd, Framework::Win32::CRect(0, 0, 1, 1), CBS_DROPDOWNLIST | WS_VSCROLL);

	PopulateDeviceList();

	m_pLayout =
		Framework::VerticalLayoutContainer(
			Framework::LayoutExpression(Framework::Win32::CLayoutWindow::CreateTextBoxBehavior(100, 15, m_pImageRadio)) +
			Framework::HorizontalLayoutContainer(
				Framework::LayoutExpression(Framework::Win32::CLayoutWindow::CreateTextBoxBehavior(100, 21, m_pImageEdit)) +
				Framework::LayoutExpression(Framework::Win32::CLayoutWindow::CreateButtonBehavior(20, 21, m_pImageBrowse))
			) +
			Framework::LayoutExpression(Framework::Win32::CLayoutWindow::CreateTextBoxBehavior(100, 15, m_pDeviceRadio)) +
			Framework::LayoutExpression(Framework::Win32::CLayoutWindow::CreateTextBoxBehavior(100, 20, m_pDeviceCombo)) +
			Framework::LayoutExpression(Framework::CLayoutStretch::Create()) +
			Framework::HorizontalLayoutContainer(
				Framework::LayoutExpression(Framework::CLayoutStretch::Create()) +
				Framework::LayoutExpression(Framework::Win32::CLayoutWindow::CreateButtonBehavior(100, 23, m_pOk)) +
				Framework::LayoutExpression(Framework::Win32::CLayoutWindow::CreateButtonBehavior(100, 23, m_pCancel))
			)
		);

	UpdateControls();

	RefreshLayout();
	m_pDeviceCombo->FixHeight(100);
}

bool CCdromSelectionWnd::WasConfirmed() const
{
	return m_nConfirmed;
}

void CCdromSelectionWnd::GetBindingInfo(CDROMBINDING* pBinding) const
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
	m_pImageRadio->SetCheck(m_nType == BINDING_IMAGE);
	m_pImageEdit->Enable(m_nType == BINDING_IMAGE);
	m_pImageBrowse->Enable(m_nType == BINDING_IMAGE);
	m_pImageEdit->SetTextA(m_sImagePath.c_str());

	m_pDeviceRadio->SetCheck(m_nType == BINDING_PHYSICAL);
	m_pDeviceCombo->Enable(m_nType == BINDING_PHYSICAL);

	int nDeviceIndex = m_pDeviceCombo->FindItemData(m_nPhysicalDevice);
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
	uint32 nDrives = GetLogicalDrives();
	for(uint32 i = 0; i < 32; i++)
	{
		if((nDrives & (1 << i)) == 0) continue;

		TCHAR sDeviceRoot[32];
		_sntprintf(sDeviceRoot, countof(sDeviceRoot), _T("%c:\\"), _T('A') + i);
		if(GetDriveType(sDeviceRoot) != DRIVE_CDROM) continue;

		TCHAR sDevicePath[32];
		_sntprintf(sDevicePath, countof(sDevicePath), _T("\\\\.\\%c:"), _T('A') + i);

		TCHAR sVendorId[256];
		TCHAR sProductId[256];
		_tcscpy(sVendorId, _T(""));
		_tcscpy(sProductId, _T("(Unknown device)"));

		HANDLE hDevice = CreateFile(sDevicePath, GENERIC_READ, FILE_SHARE_READ, NULL, OPEN_EXISTING, FILE_FLAG_NO_BUFFERING, NULL);
		if(hDevice != INVALID_HANDLE_VALUE)
		{
			STORAGE_PROPERTY_QUERY Query;
			memset(&Query, 0, sizeof(STORAGE_PROPERTY_QUERY));
			Query.PropertyId	= StorageDeviceProperty;
			Query.QueryType		= PropertyStandardQuery;

			DWORD nRetSize = 0;
			uint8 nBuffer[512];
			if(DeviceIoControl(hDevice, IOCTL_STORAGE_QUERY_PROPERTY, &Query, sizeof(STORAGE_PROPERTY_QUERY), &nBuffer, 512, &nRetSize, NULL) != 0)
			{
				auto pDesc = reinterpret_cast<STORAGE_DEVICE_DESCRIPTOR*>(nBuffer);

				_tcsncpy(sVendorId, GetVendorId(pDesc) != NULL ? string_cast<std::tstring>(GetVendorId(pDesc)).c_str() : _T(""), 256);
				_tcsncpy(sProductId, GetProductId(pDesc) != NULL ? string_cast<std::tstring>(GetProductId(pDesc)).c_str() : _T(""), 256);

				if(GetVendorId(pDesc) != NULL)
				{
					_tcscat(sVendorId, _T(" "));
				}
			}

			CloseHandle(hDevice);
		}

		TCHAR sDisplay[256];
		_sntprintf(sDisplay, countof(sDisplay), _T("%s - %s%s"),  sDeviceRoot, sVendorId, sProductId);

		unsigned int nIndex = m_pDeviceCombo->AddString(sDisplay);
		m_pDeviceCombo->SetItemData(nIndex, i);
	}
}

void CCdromSelectionWnd::SelectImage()
{
	Framework::Win32::CFileDialog Dialog;

	Dialog.m_OFN.lpstrFilter = DISKIMAGE_FILTER;

	if(Dialog.SummonOpen(m_hWnd) != IDOK)
	{
		return;
	}

	std::string sPath(string_cast<std::string>(Dialog.GetPath()));
	m_sImagePath = sPath.c_str();
}
