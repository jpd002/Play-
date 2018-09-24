#include "VfsDevice.h"
#include "AppConfig.h"
#include "PS2VM_Preferences.h"
#include "vfsdiscselectordialog.h"

#include <QFileDialog>
#include <QStorageInfo>
#include "QStringUtils.h"

///////////////////////////////////////////
//CDirectoryDevice Implementation
///////////////////////////////////////////

CDirectoryDevice::CDirectoryDevice(const char* name, const char* preference)
{
	m_name = name;
	m_preference = preference;
	m_value = CAppConfig::GetInstance().GetPreferencePath(preference);
}

const char* CDirectoryDevice::GetDeviceName()
{
	return m_name;
}

const char* CDirectoryDevice::GetBindingType()
{
	return "Directory";
}

QString CDirectoryDevice::GetBinding()
{
	return PathToQString(m_value);
}

void CDirectoryDevice::Save()
{
	CAppConfig::GetInstance().SetPreferencePath(m_preference, m_value);
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
	//Detect the binding type from the path format
	{
		auto pathString = PathToQString(path);
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
	}
	m_imagePath = path;
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

QString CCdrom0Device::GetBinding()
{
	if(m_imagePath.empty())
	{
		return "(None)";
	}
	else
	{
		return PathToQString(m_imagePath);
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
