#include "VfsDevice.h"
#include "AppConfig.h"
#include "PS2VM_Preferences.h"
#include "vfsdiscselectordialog.h"

#include <QFileDialog>
#include <QStorageInfo>

///////////////////////////////////////////
//CDirectoryDevice Implementation
///////////////////////////////////////////

CDirectoryDevice::CDirectoryDevice(const char* name, const char* preference)
{
	m_name = name;
	m_preference = preference;
	m_value = CAppConfig::GetInstance().GetPreferenceString(preference);
}

const char* CDirectoryDevice::GetDeviceName()
{
	return m_name;
}

const char* CDirectoryDevice::GetBindingType()
{
	return "Directory";
}

std::string CDirectoryDevice::GetBinding()
{
	return m_value;
}

void CDirectoryDevice::Save()
{
	CAppConfig::GetInstance().SetPreferenceString(m_preference, m_value.c_str());
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
		m_value = fileName.toStdString();
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
	auto path = CAppConfig::GetInstance().GetPreferencePath(PREF_PS2_CDROM0_PATH);
	auto pathString = QString(path.native().c_str());
	//Detect the binding type from the path format
	if(
		pathString.startsWith("\\\\.\\", Qt::CaseInsensitive) ||
		pathString.startsWith("/dev/", Qt::CaseInsensitive))
	{
		m_bindingType = CCdrom0Device::BINDING_PHYSICAL;
	}
	else
	{
		m_bindingType = CCdrom0Device::BINDING_IMAGE;
	}
	m_imagePath = path.native();
}

const char* CCdrom0Device::GetDeviceName()
{
	return "cdrom0";
}

const char* CCdrom0Device::GetBindingType()
{
	if(m_bindingType == CCdrom0Device::BINDING_PHYSICAL)
	{
		return "Physical Device";
	}
	if(m_bindingType == CCdrom0Device::BINDING_IMAGE)
	{
		return "Disk Image";
	}
	return "";
}

std::string CCdrom0Device::GetBinding()
{
	if(m_imagePath.empty())
	{
		return "(None)";
	}
	else
	{
		return m_imagePath;
	}
}

void CCdrom0Device::Save()
{
	CAppConfig::GetInstance().SetPreferencePath(PREF_PS2_CDROM0_PATH, m_imagePath);
}

bool CCdrom0Device::RequestModification(QWidget* parent)
{
	auto vfsds = new VFSDiscSelectorDialog(m_imagePath, m_bindingType, parent);
	VFSDiscSelectorDialog::connect(vfsds, &VFSDiscSelectorDialog::onFinish, [=](QString res, BINDINGTYPE type) {
		m_bindingType = type;
		m_imagePath = res.toStdString();
	});
	bool res = QDialog::Accepted == vfsds->exec();
	delete vfsds;
	return res;
}
