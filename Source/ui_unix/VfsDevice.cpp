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

const char* CDirectoryDevice::GetBinding()
{
	return m_value.c_str();
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
	auto path = QString(CAppConfig::GetInstance().GetPreferenceString(PS2VM_CDROM0PATH));
	//Detect the binding type from the path format
	if(path.startsWith("\\\\.\\", Qt::CaseInsensitive) || path.startsWith("/dev/", Qt::CaseInsensitive))
	{
		m_bindingType = CCdrom0Device::BINDING_PHYSICAL;
	}
	else
	{
		m_bindingType = CCdrom0Device::BINDING_IMAGE;
	}
	m_imagePath = path.toStdString();
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

const char* CCdrom0Device::GetBinding()
{
	if(m_imagePath.empty())
	{
		return "(None)";
	}
	else
	{
		return m_imagePath.c_str();
	}
}

void CCdrom0Device::Save()
{
	CAppConfig::GetInstance().SetPreferenceString(PS2VM_CDROM0PATH, m_imagePath.c_str());
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
