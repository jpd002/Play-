#include <shlobj.h>
#include "resource.h"
#include "VFSManagerWnd.h"
#include "../AppConfig.h"
#include "../PS2VM_Preferences.h"
#include "CdromSelectionWnd.h"
#include "string_cast.h"
#include "string_format.h"

CVFSManagerWnd::CVFSManagerWnd(HWND parentWindow)
: CDialog(MAKEINTRESOURCE(IDD_VFSMANAGER), parentWindow)
{
	SetClassPtr();

	m_deviceList = Framework::Win32::CListView(GetItem(IDC_VFSMANAGER_DEVICES));
	m_deviceList.SetExtendedListViewStyle(m_deviceList.GetExtendedListViewStyle() | LVS_EX_FULLROWSELECT);

	m_devices[0] = std::make_unique<CDirectoryDevice>("mc0",  PREF_PS2_MC0_DIRECTORY);
	m_devices[1] = std::make_unique<CDirectoryDevice>("mc1",  PREF_PS2_MC1_DIRECTORY);
	m_devices[2] = std::make_unique<CDirectoryDevice>("host", PREF_PS2_HOST_DIRECTORY);
	m_devices[3] = std::make_unique<CCdrom0Device>();

	CreateListColumns();
	UpdateList();
}

long CVFSManagerWnd::OnCommand(unsigned short nID, unsigned short nCmd, HWND hSender)
{
	switch(nID)
	{
	case IDOK:
		Save();
		Destroy();
		return TRUE;
		break;
	case IDCANCEL:
		Destroy();
		return TRUE;
		break;
	}
	return FALSE;
}

LRESULT CVFSManagerWnd::OnNotify(WPARAM wParam, NMHDR* hdr)
{
	if(hdr->idFrom == IDC_VFSMANAGER_DEVICES)
	{
		switch(hdr->code)
		{
		case NM_DBLCLK:
			int nSel = m_deviceList.GetSelection();
			if(nSel != -1)
			{
				const auto deviceIterator = m_devices.find(m_deviceList.GetItemData(nSel));
				if(deviceIterator != m_devices.end())
				{
					const auto& device = deviceIterator->second;
					if(device->RequestModification(m_hWnd))
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

void CVFSManagerWnd::CreateListColumns()
{
	LVCOLUMN col;
	RECT rc = m_deviceList.GetClientRect();

	memset(&col, 0, sizeof(LVCOLUMN));
	col.pszText = _T("Device");
	col.mask	= LVCF_TEXT | LVCF_WIDTH;
	col.cx		= rc.right / 4;
	m_deviceList.InsertColumn(0, col);

	memset(&col, 0, sizeof(LVCOLUMN));
	col.pszText = _T("Binding Type");
	col.mask	= LVCF_TEXT | LVCF_WIDTH;
	col.cx		= rc.right / 4;
	m_deviceList.InsertColumn(1, col);

	memset(&col, 0, sizeof(LVCOLUMN));
	col.pszText = _T("Binding Value");
	col.mask	= LVCF_TEXT | LVCF_WIDTH;
	col.cx		= rc.right / 2;
	m_deviceList.InsertColumn(2, col);
}

void CVFSManagerWnd::UpdateList()
{
	for(const auto& devicePair : m_devices)
	{
		const auto& device = devicePair.second;
		auto key = devicePair.first;

		unsigned int index = m_deviceList.FindItemData(key);
		if(index == -1)
		{
			auto deviceName = string_cast<std::tstring>(device->GetDeviceName());

			LVITEM itm;
			memset(&itm, 0, sizeof(LVITEM));
			itm.mask		= LVIF_TEXT | LVIF_PARAM;
			itm.pszText		= const_cast<TCHAR*>(deviceName.c_str());
			itm.lParam		= key;
			index = m_deviceList.InsertItem(itm);
		}

		m_deviceList.SetItemText(index, 1, string_cast<std::tstring>(device->GetBindingType()).c_str());
		m_deviceList.SetItemText(index, 2, device->GetBinding().c_str());
	}
}

void CVFSManagerWnd::Save()
{
	for(const auto& devicePair : m_devices)
	{
		const auto& device = devicePair.second;
		device->Save();
	}
}

///////////////////////////////////////////
//CDirectoryDevice Implementation
///////////////////////////////////////////

CVFSManagerWnd::CDirectoryDevice::CDirectoryDevice(const char* name, const char* preference)
: m_name(name)
, m_preference(preference)
{
	auto path = CAppConfig::GetInstance().GetPreferenceString(m_preference);
	m_path = string_cast<std::tstring>(path);
}

const char* CVFSManagerWnd::CDirectoryDevice::GetDeviceName()
{
	return m_name;
}

const char* CVFSManagerWnd::CDirectoryDevice::GetBindingType()
{
	return "Directory";
}

std::tstring CVFSManagerWnd::CDirectoryDevice::GetBinding()
{
	return m_path;
}

bool CVFSManagerWnd::CDirectoryDevice::RequestModification(HWND hParent)
{
	BROWSEINFO bi;
	memset(&bi, 0, sizeof(BROWSEINFO));
	bi.hwndOwner	= hParent;
	bi.lpszTitle	= _T("Select new folder for device");
	bi.ulFlags		= BIF_RETURNONLYFSDIRS;
	bi.lpfn			= BrowseCallback;
	bi.lParam		= (LPARAM)this;

	auto item = SHBrowseForFolder(&bi);
	if(item == NULL)
	{
		return false;
	}

	TCHAR path[MAX_PATH];
	if(SHGetPathFromIDList(item, path) == 0)
	{
		MessageBox(hParent, _T("Invalid directory."), NULL, 16);
		CoTaskMemFree(item);
		return false;
	}

	CoTaskMemFree(item);

	m_path = path;

	return true;
}

void CVFSManagerWnd::CDirectoryDevice::Save()
{
	auto cvtPath = string_cast<std::string>(m_path);
	CAppConfig::GetInstance().SetPreferenceString(m_preference, cvtPath.c_str());
}

int CVFSManagerWnd::CDirectoryDevice::BrowseCallback(HWND hFrom, unsigned int nMsg, LPARAM lParam, LPARAM pData)
{
	auto device = reinterpret_cast<CDirectoryDevice*>(pData);
	switch(nMsg)
	{
	case BFFM_INITIALIZED:
		SendMessage(hFrom, BFFM_SETSELECTION, TRUE, reinterpret_cast<LPARAM>(device->m_path.c_str()));
		break;
	}
	return 0;
}

///////////////////////////////////////////
//CCdrom0Device Implementation
///////////////////////////////////////////

CVFSManagerWnd::CCdrom0Device::CCdrom0Device()
{
	auto cdrom0Path = CAppConfig::GetInstance().GetPreferenceString(PS2VM_CDROM0PATH);
	
	//Detect the binding type from the path format
	if(!strcmp(cdrom0Path, ""))
	{
		m_bindingType = CCdromSelectionWnd::BINDING_IMAGE;
		m_imagePath   = _T("");
	}
	else if(strlen(cdrom0Path) == 6 && !strncmp(cdrom0Path, "\\\\.\\", 4))
	{
		m_bindingType = CCdromSelectionWnd::BINDING_PHYSICAL;
		m_devicePath  = string_format("%c:\\", toupper(cdrom0Path[4]));
	}
	else
	{
		m_bindingType = CCdromSelectionWnd::BINDING_IMAGE;
		m_imagePath   = string_cast<std::tstring>(cdrom0Path);
	}
}

const char* CVFSManagerWnd::CCdrom0Device::GetDeviceName()
{
	return "cdrom0";
}

const char* CVFSManagerWnd::CCdrom0Device::GetBindingType()
{
	if(m_bindingType == CCdromSelectionWnd::BINDING_PHYSICAL)
	{
		return "Physical Device";
	}
	else if(m_bindingType == CCdromSelectionWnd::BINDING_IMAGE)
	{
		return "Disk Image";
	}
	return "";
}

std::tstring CVFSManagerWnd::CCdrom0Device::GetBinding()
{
	if(m_bindingType == CCdromSelectionWnd::BINDING_IMAGE)
	{
		if(m_imagePath.empty())
		{
			return _T("(None)");
		}
		else
		{
			return m_imagePath.c_str();
		}
	}
	else if(m_bindingType == CCdromSelectionWnd::BINDING_PHYSICAL)
	{
		return string_cast<std::tstring>(m_devicePath.c_str());
	}
	return _T("");
}

bool CVFSManagerWnd::CCdrom0Device::RequestModification(HWND hParent)
{
	CCdromSelectionWnd::CDROMBINDING binding;
	binding.type           = static_cast<CCdromSelectionWnd::BINDINGTYPE>(m_bindingType);
	binding.imagePath      = m_imagePath;
	binding.physicalDevice = m_devicePath[0] - 'A';

	CCdromSelectionWnd selectionWnd(hParent, _T("Modify cdrom0 Binding"));
	selectionWnd.SetBindingInfo(binding);
	selectionWnd.DoModal();
	if(!selectionWnd.WasConfirmed()) return false;

	binding = selectionWnd.GetBindingInfo();

	m_bindingType = binding.type;
	m_imagePath   = binding.imagePath;
	m_devicePath  = string_format("%c:\\", binding.physicalDevice + 'A');

	return true;
}

void CVFSManagerWnd::CCdrom0Device::Save()
{
	if(m_bindingType == CCdromSelectionWnd::BINDING_IMAGE)
	{
		auto cvtImagePath = string_cast<std::string>(m_imagePath);
		CAppConfig::GetInstance().SetPreferenceString(PS2VM_CDROM0PATH, cvtImagePath.c_str());
	}
	else if(m_bindingType == CCdromSelectionWnd::BINDING_PHYSICAL)
	{
		auto devicePath = string_format("\\\\.\\%c:", m_devicePath[0]);
		CAppConfig::GetInstance().SetPreferenceString(PS2VM_CDROM0PATH, devicePath.c_str());
	}
}
