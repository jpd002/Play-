#include "CdromSelectionWnd.h"
#include "win32/FileDialog.h"
#include "resource.h"
#include "string_cast.h"
#include "FileFilters.h"
#include <winioctl.h>

static const char* GetVendorId(const _STORAGE_DEVICE_DESCRIPTOR* descriptor)
{
	return descriptor->VendorIdOffset != 0 ? reinterpret_cast<const char*>(descriptor) + descriptor->VendorIdOffset : NULL;
}

static const char* GetProductId(const _STORAGE_DEVICE_DESCRIPTOR* descriptor)
{
	return descriptor->ProductIdOffset != 0 ? reinterpret_cast<const char*>(descriptor) + descriptor->ProductIdOffset : NULL;
}

CCdromSelectionWnd::CCdromSelectionWnd(HWND parentWindow, const TCHAR* title)
: CDialog(MAKEINTRESOURCE(IDD_CDROM_SELECTION), parentWindow)
{
	SetClassPtr();
	SetText(title);

	m_imageRadio = Framework::Win32::CButton(GetItem(IDC_CDROM_SELECTION_IMAGE));
	m_imageEdit = Framework::Win32::CEdit(GetItem(IDC_CDROM_SELECTION_IMAGEPATH));
	m_imageBrowse = Framework::Win32::CButton(GetItem(IDC_CDROM_SELECTION_BROWSEIMAGE));

	m_deviceRadio = Framework::Win32::CButton(GetItem(IDC_CDROM_SELECTION_DEVICE));
	m_deviceCombo = Framework::Win32::CComboBox(GetItem(IDC_CDROM_SELECTION_DEVICELIST));

	PopulateDeviceList();
	UpdateControls();
}

bool CCdromSelectionWnd::WasConfirmed() const
{
	return m_confirmed;
}

CCdromSelectionWnd::CDROMBINDING CCdromSelectionWnd::GetBindingInfo() const
{
	return m_binding;
}

void CCdromSelectionWnd::SetBindingInfo(const CDROMBINDING& binding)
{
	m_binding = binding;
	UpdateControls();
}

long CCdromSelectionWnd::OnCommand(unsigned short nID, unsigned short nCmd, HWND hFrom)
{
	switch(nID)
	{
	case IDOK:
		if(m_binding.type == BINDING_IMAGE)
		{
			if(m_binding.imagePath.empty())
			{
				MessageBox(m_hWnd, _T("Please select a disk image to bind with this device."), NULL, 16);
				return FALSE;
			}
		}

		m_confirmed = true;
		Destroy();
		return FALSE;
		break;

	case IDCANCEL:
		Destroy();
		return FALSE;
		break;

	case IDC_CDROM_SELECTION_IMAGE:
		m_binding.type = BINDING_IMAGE;
		UpdateControls();
		break;

	case IDC_CDROM_SELECTION_BROWSEIMAGE:
		SelectImage();
		UpdateControls();
		break;

	case IDC_CDROM_SELECTION_DEVICE:
		m_binding.type = BINDING_PHYSICAL;
		m_binding.physicalDevice = m_deviceCombo.GetItemData(m_deviceCombo.GetSelection());
		UpdateControls();
		break;

	case IDC_CDROM_SELECTION_DEVICELIST:
		if(nCmd == CBN_SELCHANGE)
		{
			m_binding.physicalDevice = m_deviceCombo.GetItemData(m_deviceCombo.GetSelection());
			UpdateControls();
		}
		break;
	}
	return TRUE;
}

void CCdromSelectionWnd::UpdateControls()
{
	m_imageRadio.SetCheck(m_binding.type == BINDING_IMAGE);
	m_imageEdit.Enable(m_binding.type == BINDING_IMAGE);
	m_imageEdit.SetText(m_binding.imagePath.c_str());
	m_imageBrowse.Enable(m_binding.type == BINDING_IMAGE);

	m_deviceRadio.SetCheck(m_binding.type == BINDING_PHYSICAL);
	m_deviceCombo.Enable(m_binding.type == BINDING_PHYSICAL);

	int nDeviceIndex = m_deviceCombo.FindItemData(m_binding.physicalDevice);
	m_deviceCombo.SetSelection((nDeviceIndex != -1) ? nDeviceIndex : 0);
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

		unsigned int nIndex = m_deviceCombo.AddString(sDisplay);
		m_deviceCombo.SetItemData(nIndex, i);
	}
}

void CCdromSelectionWnd::SelectImage()
{
	Framework::Win32::CFileDialog dialog;
	dialog.m_OFN.lpstrFilter = DISKIMAGE_FILTER;

	if(dialog.SummonOpen(m_hWnd) != IDOK)
	{
		return;
	}

	m_binding.imagePath = dialog.GetPath();
}
