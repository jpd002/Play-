#ifndef VFSDEVICE_H
#define VFSDEVICE_H
#include <QWidget>
#include <map>

class CDevice
{
public:
    CDevice();
    virtual						~CDevice();
    virtual const char*			GetDeviceName() = 0 ;
    virtual const char*			GetBindingType() = 0;
    virtual const char*			GetBinding() = 0;
    virtual bool                RequestModification(QWidget*) = 0;
    virtual void				Save() = 0;
    enum BINDINGTYPE
    {
        BINDING_IMAGE = 1,
        BINDING_PHYSICAL = 2,
    };
    typedef std::map<unsigned int, CDevice *> DeviceList;
};



class CDirectoryDevice : public CDevice
{
public:
    CDirectoryDevice(const char*, const char*);
    virtual                     ~CDirectoryDevice();

    virtual const char*			GetDeviceName() override;
    virtual const char*			GetBindingType() override;
    virtual const char*			GetBinding() override;
    virtual bool                RequestModification(QWidget*) override;
    virtual void				Save() override;
private:

    const char*                 m_sName;
    const char*                 m_sPreference;
    std::string                 m_sValue;
};

class CCdrom0Device : public CDevice
{
public:
    CCdrom0Device();
    virtual                     ~CCdrom0Device();

    virtual const char*			GetDeviceName() override;
    virtual const char*			GetBindingType() override;
    virtual const char*			GetBinding() override;
    virtual bool                RequestModification(QWidget*) override;
    virtual void				Save() override;

private:
    std::string                 m_sImagePath;
    BINDINGTYPE                 m_nBindingType;
};

#endif // VFSDEVICE_H
