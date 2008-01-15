#ifndef _DIRECTORYDEVICE_H_
#define _DIRECTORYDEVICE_H_

#include <string>
#include "Ioman_Device.h"

class CDirectoryDevice : public Iop::Ioman::CDevice
{
public:
                                    CDirectoryDevice(const char*);
    virtual                         ~CDirectoryDevice();
    virtual Framework::CStream*     GetFile(uint32, const char*);

private:
    std::string                     m_basePathPreferenceName;
};

#endif
