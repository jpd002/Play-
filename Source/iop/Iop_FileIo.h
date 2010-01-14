#ifndef _IOP_FILEIO_H_
#define _IOP_FILEIO_H_

#include "Iop_SifMan.h"
#include "Iop_Module.h"

namespace Iop
{
    class CIoman;

    class CFileIo : public CSifModule, public CModule
    {
    public:
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

            struct ACTIVATECOMMAND
            {
                COMMANDHEADER   header;
                char            device[256];
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

            struct ACTIVATEREPLY
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

                                CFileIo(CSifMan&, CIoman&);
        virtual                 ~CFileIo();

        virtual std::string     GetId() const;
        virtual std::string     GetFunctionName(unsigned int) const;
        virtual void            Invoke(CMIPS&, unsigned int);
        virtual bool            Invoke(uint32, uint32*, uint32, uint32*, uint32, uint8*);

    private:
        CSifMan&                m_sifMan;
        CIoman&                 m_ioman;
        CFileIoHandler*         m_fileIoHandler;
    };
}

#endif
