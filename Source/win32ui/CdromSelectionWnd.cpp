#include "CdromSelectionWnd.h"
#include "HorizontalLayout.h"
#include "LayoutStretch.h"
#include "win32/LayoutWindow.h"
#include "win32/FileDialog.h"
#include "../ISO9660/ISO9660.h"
#include "string_cast.h"
#include "StdStream.h"
#include "PtrMacro.h"
#include <winioctl.h>

#define CLSNAME		_T("CdromSelectionWnd")
#define WNDSTYLE	(WS_CAPTION | WS_POPUP | WS_CLIPSIBLINGS | WS_CLIPCHILDREN | WS_SYSMENU)
#define WNDSTYLEEX	(WS_EX_DLGMODALFRAME)

using namespace Framework;
using namespace std;

//------------------------------------------------------
//Some definitions from the DDK (and +)
//------------------------------------------------------

#pragma pack(push, 4)

typedef enum _STORAGE_QUERY_TYPE 
{
	PropertyStandardQuery = 0,
	PropertyExistsQuery,
	PropertyMaskQuery,
	PropertyQueryMaxDefined
} STORAGE_QUERY_TYPE, *PSTORAGE_QUERY_TYPE;

typedef enum _STORAGE_PROPERTY_ID
{
	StorageDeviceProperty = 0,
	StorageAdapterProperty
} STORAGE_PROPERTY_ID, *PSTORAGE_PROPERTY_ID;

typedef struct _STORAGE_PROPERTY_QUERY
{
	STORAGE_PROPERTY_ID			PropertyId;
	STORAGE_QUERY_TYPE			QueryType;
	UCHAR						AdditionalParameters[1];
} STORAGE_PROPERTY_QUERY, *PSTORAGE_PROPERTY_QUERY;

#define IOCTL_STORAGE_QUERY_PROPERTY   CTL_CODE(IOCTL_STORAGE_BASE, 0x0500, METHOD_BUFFERED, FILE_ANY_ACCESS)

typedef struct _STORAGE_DEVICE_DESCRIPTOR
{
	ULONG						Version;
	ULONG						Size;
	UCHAR						DeviceType;
	UCHAR						DeviceTypeModifier;
	BOOLEAN						RemovableMedia;
	BOOLEAN						CommandQueueing;
	ULONG						VendorIdOffset;
	ULONG						ProductIdOffset;
	ULONG						ProductRevisionOffset;
	ULONG						SerialNumberOffset;
	STORAGE_BUS_TYPE			BusType;
	ULONG						RawPropertiesLength;
	UCHAR						RawDeviceProperties[1];
	const char* GetVendorId()	{ return VendorIdOffset != 0 ? (char*)this + VendorIdOffset : NULL; }
	const char* GetProductId()	{ return ProductIdOffset != 0 ? (char*)this + ProductIdOffset : NULL; }
} STORAGE_DEVICE_DESCRIPTOR, *PSTORAGE_DEVICE_DESCRIPTOR;

#pragma pack(pop)

//------------------------------------------------------

CCdromSelectionWnd::CCdromSelectionWnd(HWND hParent, const TCHAR* sTitle, CDROMBINDING* pInitBinding) :
CModalWindow(hParent)
{
	RECT rc;
	CHorizontalLayout* pSubLayout0;
	CHorizontalLayout* pSubLayout1;

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

	SetRect(&rc, 0, 0, 300, 170);

	Create(WNDSTYLEEX, CLSNAME, sTitle, WNDSTYLE, &rc, hParent, NULL);
	SetClassPtr();

	SetRect(&rc, 0, 0, 1, 1);

	m_pOk			= new Win32::CButton(_T("OK"), m_hWnd, &rc);
	m_pCancel		= new Win32::CButton(_T("Cancel"), m_hWnd, &rc);

	m_pImageRadio	= new Win32::CButton(_T("ISO9660 Disk Image"), m_hWnd, &rc, BS_RADIOBUTTON);
	m_pDeviceRadio	= new Win32::CButton(_T("Physical CD/DVD Reader Device"), m_hWnd, &rc, BS_RADIOBUTTON);

	m_pImageEdit	= new Win32::CEdit(m_hWnd, &rc, _T(""), ES_READONLY);
	m_pImageBrowse	= new Win32::CButton(_T("..."), m_hWnd, &rc);
	m_pDeviceCombo	= new Win32::CComboBox(m_hWnd, &rc, CBS_DROPDOWNLIST | WS_VSCROLL);

	PopulateDeviceList();

	pSubLayout0 = new CHorizontalLayout;
	pSubLayout0->InsertObject(new CLayoutStretch);
    pSubLayout0->InsertObject(Win32::CLayoutWindow::CreateButtonBehavior(100, 23, m_pOk));
    pSubLayout0->InsertObject(Win32::CLayoutWindow::CreateButtonBehavior(100, 23, m_pCancel));
	pSubLayout0->SetVerticalStretch(0);

	pSubLayout1 = new CHorizontalLayout;
    pSubLayout1->InsertObject(Win32::CLayoutWindow::CreateTextBoxBehavior(100, 21, m_pImageEdit));
    pSubLayout1->InsertObject(Win32::CLayoutWindow::CreateButtonBehavior(20, 21, m_pImageBrowse));
	pSubLayout1->SetVerticalStretch(0);

	m_pLayout = new CVerticalLayout;
	m_pLayout->InsertObject(Win32::CLayoutWindow::CreateTextBoxBehavior(100, 15, m_pImageRadio));
	m_pLayout->InsertObject(pSubLayout1);
	m_pLayout->InsertObject(Win32::CLayoutWindow::CreateTextBoxBehavior(100, 15, m_pDeviceRadio));
	m_pLayout->InsertObject(Win32::CLayoutWindow::CreateTextBoxBehavior(100, 20, m_pDeviceCombo));
	m_pLayout->InsertObject(new CLayoutStretch);
	m_pLayout->InsertObject(pSubLayout0);

	UpdateControls();

	RefreshLayout();
	m_pDeviceCombo->FixHeight(100);
}

CCdromSelectionWnd::~CCdromSelectionWnd()
{
	DELETEPTR(m_pLayout);
}

bool CCdromSelectionWnd::WasConfirmed()
{
	return m_nConfirmed;
}

void CCdromSelectionWnd::GetBindingInfo(CDROMBINDING* pBinding)
{
	if(pBinding == NULL) return;

	pBinding->nType				= m_nType;
	pBinding->sImagePath		= m_sImagePath;
	pBinding->nPhysicalDevice	= m_nPhysicalDevice;
}

long CCdromSelectionWnd::OnCommand(unsigned short nID, unsigned short nCmd, HWND hFrom)
{
	if(hFrom == m_pOk->m_hWnd)
	{
		if(m_nType == BINDING_IMAGE)
		{
			if(strlen(m_sImagePath) == 0)
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
	m_pImageEdit->SetTextA(m_sImagePath);

	m_pDeviceRadio->SetCheck(m_nType == BINDING_PHYSICAL);
	m_pDeviceCombo->Enable(m_nType == BINDING_PHYSICAL);

	nDeviceIndex = m_pDeviceCombo->FindItemData(m_nPhysicalDevice);
	m_pDeviceCombo->SetSelection(nDeviceIndex != -1 ? nDeviceIndex : 0);
}

void CCdromSelectionWnd::RefreshLayout()
{
	RECT rc;

	GetClientRect(&rc);

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

				_tcsncpy(sVendorId, pDesc->GetVendorId() != NULL ? string_cast<tstring>(pDesc->GetVendorId()).c_str() : _T(""), 256);
				_tcsncpy(sProductId, pDesc->GetProductId() != NULL ? string_cast<tstring>(pDesc->GetProductId()).c_str() : _T(""), 256);

				if(pDesc->GetVendorId() != NULL)
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
	CISO9660* pISO;
	CStdStream* pStream;

	Dialog.m_OFN.lpstrFilter = _T("ISO9660 Disk Images (*.iso; *.bin)\0*.iso;*.bin\0All files (*.*)\0*.*\0");

	if(Dialog.Summon(m_hWnd) != IDOK)
	{
		return;
	}

    string sPath(string_cast<string>(Dialog.m_sFile));

	try
	{
		pStream = new CStdStream(fopen(sPath.c_str(), "rb"));
	}
	catch(...)
	{
		CWindow::MessageBoxFormat(m_hWnd, 16, NULL, _T("Couldn't open file '%s' for reading."), Dialog.m_sFile);
		return;
	}

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

	m_sImagePath = sPath.c_str();
}
