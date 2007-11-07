#ifndef _IOP_IOMAN_H_
#define _IOP_IOMAN_H_

#include <map>
#include "Iop_Module.h"
#include "Ioman_Device.h"
#include "Stream.h"

namespace Iop
{
    class CIoman : public CModule
    {
    public:
                                CIoman(uint8*);
        virtual                 ~CIoman();
        
        virtual std::string     GetId() const;
        virtual void            Invoke(CMIPS&, unsigned int);

        void                    RegisterDevice(const char*, Ioman::CDevice*);

        uint32                  Open(uint32, const char*);

    private:
        typedef std::map<uint32, Framework::CStream*> FileMapType;
        typedef std::map<std::string, Ioman::CDevice*> DeviceMapType;

        void                    Open(CMIPS&);

        FileMapType             m_files;
        DeviceMapType           m_devices;
        uint8*                  m_ram;
    };
}

#endif
