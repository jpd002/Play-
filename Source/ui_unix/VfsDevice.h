#pragma once

#include <QWidget>
#include <map>

class CDevice
{
public:
	virtual ~CDevice() = default;
	virtual const char* GetDeviceName() = 0;
	virtual const char* GetBindingType() = 0;
	virtual std::string GetBinding() = 0;
	virtual bool RequestModification(QWidget*) = 0;
	virtual void Save() = 0;
};
typedef std::map<unsigned int, CDevice*> DeviceList;

class CDirectoryDevice : public CDevice
{
public:
	CDirectoryDevice(const char*, const char*);
	virtual ~CDirectoryDevice() = default;

	const char* GetDeviceName() override;
	const char* GetBindingType() override;
	std::string GetBinding() override;
	bool RequestModification(QWidget*) override;
	void Save() override;

private:
	const char* m_name;
	const char* m_preference;
	std::string m_value;
};

class CCdrom0Device : public CDevice
{
public:
	enum BINDINGTYPE
	{
		BINDING_IMAGE = 1,
		BINDING_PHYSICAL = 2,
	};

	CCdrom0Device();
	virtual ~CCdrom0Device() = default;

	const char* GetDeviceName() override;
	const char* GetBindingType() override;
	std::string GetBinding() override;
	bool RequestModification(QWidget*) override;
	void Save() override;

private:
	std::string m_imagePath;
	BINDINGTYPE m_bindingType;
};
