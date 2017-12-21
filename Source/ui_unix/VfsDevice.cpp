#include "VfsDevice.h"
#include "AppConfig.h"
#include "PS2VM_Preferences.h"
#include "vfsdiscselectordialog.h"

#include <QFileDialog>
#include <QStorageInfo>

///////////////////////////////////////////
//CDirectoryDevice Implementation
///////////////////////////////////////////

CDirectoryDevice::CDirectoryDevice(const char* sName, const char* sPreference)
{
	m_sName = sName;
	m_sPreference = sPreference;
	m_sValue = CAppConfig::GetInstance().GetPreferenceString(m_sPreference);
}

const char* CDirectoryDevice::GetDeviceName()
{
	return m_sName;
}

const char* CDirectoryDevice::GetBindingType()
{
	return "Directory";
}

const char* CDirectoryDevice::GetBinding()
{
	return m_sValue.c_str();
}

void CDirectoryDevice::Save()
{
	CAppConfig::GetInstance().SetPreferenceString(m_sPreference, m_sValue.c_str());
}

bool CDirectoryDevice::RequestModification(QWidget* parent)
{
	QFileDialog dialog(parent);
	dialog.setFileMode(QFileDialog::Directory);
	dialog.setOption(QFileDialog::ShowDirsOnly);
	dialog.setOption(QFileDialog::DontResolveSymlinks);
	if(dialog.exec())
	{
		QString fileName = dialog.selectedFiles().first();
		m_sValue = fileName.toStdString();
		return true;
	}
	else
	{
		return false;
	}
}

///////////////////////////////////////////
//CCdrom0Device Implementation
///////////////////////////////////////////

CCdrom0Device::CCdrom0Device()
{
	const char* sPath = CAppConfig::GetInstance().GetPreferenceString(PS2VM_CDROM0PATH);

	//Detect the binding type from the path format
	QString m_sPath(sPath);
	if(m_sPath.startsWith("\\\\.\\", Qt::CaseInsensitive) || m_sPath.startsWith("/dev/", Qt::CaseInsensitive))
	{
		m_nBindingType = CCdrom0Device::BINDING_PHYSICAL;
		m_sImagePath = sPath;
	}
	else
	{
		m_nBindingType = CCdrom0Device::BINDING_IMAGE;
		if(!strcmp(sPath, ""))
		{
			m_sImagePath = "";
		}
		else
		{
			m_sImagePath = sPath;
		}
	}
}

const char* CCdrom0Device::GetDeviceName()
{
	return "cdrom0";
}

const char* CCdrom0Device::GetBindingType()
{
	if(m_nBindingType == CCdrom0Device::BINDING_PHYSICAL)
	{
		return "Physical Device";
	}
	if(m_nBindingType == CCdrom0Device::BINDING_IMAGE)
	{
		return "Disk Image";
	}
	return "";
}

const char* CCdrom0Device::GetBinding()
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

void CCdrom0Device::Save()
{
	CAppConfig::GetInstance().SetPreferenceString(PS2VM_CDROM0PATH, m_sImagePath.c_str());
}

bool CCdrom0Device::RequestModification(QWidget* parent)
{
	bool res = false;
	auto vfsds = new VFSDiscSelectorDialog(m_sImagePath, m_nBindingType, parent);
	VFSDiscSelectorDialog::connect(vfsds, &VFSDiscSelectorDialog::onFinish, [=](QString res, BINDINGTYPE type) {
		m_nBindingType = type;
		m_sImagePath = res.toStdString();
	});
	res = QDialog::Accepted == vfsds->exec();
	delete vfsds;
	return res;
}
