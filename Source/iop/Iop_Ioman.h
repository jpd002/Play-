#ifndef _IOP_IOMAN_H_
#define _IOP_IOMAN_H_

#include <map>
#include "Iop_SifMan.h"
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

        class CFileIoHandler
        {
        public:
                            CFileIoHandler(CIoman*);
            virtual         ~CFileIoHandler() {}
            virtual void    Invoke(uint32, uint32*, uint32, uint32*, uint32, uint8*) = 0;

        protected:
            CIoman*         m_ioman;
        };

        class CFileIoHandlerBasic : public CFileIoHandler
        {
        public:
                            CFileIoHandlerBasic(CIoman*);
            virtual void    Invoke(uint32, uint32*, uint32, uint32*, uint32, uint8*);
        };

        class CFileIoHandler3100 : public CFileIoHandler
        {
        public:
                            CFileIoHandler3100(CIoman*, CSifMan&);
            virtual void    Invoke(uint32, uint32*, uint32, uint32*, uint32, uint8*);

        private:
            struct COMMANDHEADER
            {
                uint32  semaphoreId;
                uint32  resultPtr;
                uint32  resultSize;
            };

            struct OPENCOMMAND
            {
                COMMANDHEADER   header;
                uint32          flags;
                uint32          somePtr1;
                char            fileName[256];
            };

            struct CLOSECOMMAND
            {
                COMMANDHEADER   header;
                uint32          fd;
            };

            struct READCOMMAND
            {
                COMMANDHEADER   header;
                uint32          fd;
                uint32          buffer;
                uint32          size;
            };

            struct SEEKCOMMAND
            {
                COMMANDHEADER   header;
                uint32          fd;
                uint32          offset;
                uint32          whence;
            };

            struct REPLYHEADER
            {
                uint32  semaphoreId;
                uint32  commandId;
                uint32  resultPtr;
                uint32  resultSize;
            };

            struct OPENREPLY
            {
                REPLYHEADER     header;
                uint32          result;
                uint32          unknown2;
                uint32          unknown3;
                uint32          unknown4;
            };

            struct CLOSEREPLY
            {
                REPLYHEADER     header;
                uint32          result;
                uint32          unknown2;
                uint32          unknown3;
                uint32          unknown4;
            };

            struct READREPLY
            {
                REPLYHEADER     header;
                uint32          result;
                uint32          unknown2;
                uint32          unknown3;
                uint32          unknown4;
            };

            struct SEEKREPLY
            {
                REPLYHEADER     header;
                uint32          result;
                uint32          unknown2;
                uint32          unknown3;
                uint32          unknown4;
            };

            void            CopyHeader(REPLYHEADER&, const COMMANDHEADER&);
            CSifMan&        m_sifMan;
        };

		enum SIF_MODULE_ID
		{
			SIF_MODULE_ID	= 0x80000001
		};

                                CIoman(uint8*, CSifMan&);
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
        CSifMan&                m_sifMan;
        uint32                  m_nextFileHandle;
        CFileIoHandler*         m_fileIoHandler;
    };
}

#endif
