#ifndef _IOP_IOMAN_H_
#define _IOP_IOMAN_H_

#include <map>
#include "../Sif.h"
#include "Iop_Module.h"
#include "Ioman_Device.h"
#include "Stream.h"

namespace Iop
{
    class CIoman : public CModule, public CSifModule
    {
    public:
        class CFile
        {
        public:
                        CFile(uint32, CIoman&);
            virtual     ~CFile();

                        operator uint32();
        private:
            uint32      m_handle;
            CIoman&     m_ioman;
        };

		enum SIF_MODULE_ID
		{
			SIF_MODULE_ID	= 0x80000001
		};

                                CIoman(uint8*, CSIF&);
        virtual                 ~CIoman();
        
        virtual std::string     GetId() const;
        virtual void            Invoke(CMIPS&, unsigned int);
        virtual void            Invoke(uint32, uint32*, uint32, uint32*, uint32, uint8*);

        void                    RegisterDevice(const char*, Ioman::CDevice*);

        uint32                  Open(uint32, const char*);
        uint32                  Close(uint32);
        uint32                  Read(uint32, uint32, void*);
        uint32                  Write(uint32, uint32, void*);
        uint32                  Seek(uint32, uint32, uint32);
        uint32                  DelDrv(const char*);

        Framework::CStream*     GetFileStream(uint32);

    private:
        typedef std::map<uint32, Framework::CStream*> FileMapType;
        typedef std::map<std::string, Ioman::CDevice*> DeviceMapType;

        void                    Open(CMIPS&);

        FileMapType             m_files;
        DeviceMapType           m_devices;
        uint8*                  m_ram;
        uint32                  m_nextFileHandle;
    };
}

#endif
