#ifndef _IOP_SIFCMD_H_
#define _IOP_SIFCMD_H_

#include "Iop_Module.h"
#include "Iop_SifMan.h"
#include "Iop_SifDynamic.h"
#include "Iop_SysMem.h"
#include "zip/ZipArchiveWriter.h"
#include "zip/ZipArchiveReader.h"

class CIopBios;

namespace Iop
{
    class CSifCmd : public CModule
    {
    public:
                                CSifCmd(CIopBios&, CSifMan&, CSysmem&, uint8*);
        virtual                 ~CSifCmd();

        virtual std::string     GetId() const;
		virtual std::string		GetFunctionName(unsigned int) const;
        virtual void            Invoke(CMIPS&, unsigned int);

        void                    ProcessInvocation(uint32, uint32, uint32*, uint32);

	    void					LoadState(Framework::CZipArchiveReader&);
	    void					SaveState(Framework::CZipArchiveWriter&);

    private:
        typedef std::list<CSifDynamic*> DynamicModuleList;

        struct SIFRPCDATAQUEUE
        {
            uint32      threadId;
            uint32      active;
            uint32      serverDataLink;
            uint32      serverDataStart;
            uint32      serverDataEnd;
            uint32      queueNext;
        };

        struct SIFRPCSERVERDATA
        {
            uint32      serverId;

            uint32      function;
            uint32      buffer;
            uint32      size;

            uint32      cfunction;
            uint32      cbuffer;
            uint32      csize;

            uint32      queueAddr;
        };

        void                    ClearServers();
        void                    BuildExportTable();

        uint32                  SifSendCmd(uint32, uint32, uint32, uint32, uint32, uint32);
        uint32                  SifBindRpc(uint32, uint32, uint32);
        void                    SifRegisterRpc(CMIPS&);
        uint32                  SifCheckStatRpc(uint32);
        void                    SifSetRpcQueue(uint32, uint32);
        void                    SifRpcLoop(uint32);
        void                    ReturnFromRpcInvoke(CMIPS&);

        CIopBios&               m_bios;
        CSifMan&                m_sifMan;
        CSysmem&                m_sysMem;
        uint8*                  m_ram;
        uint32                  m_memoryBufferAddr;
        uint32                  m_invokeParamsAddr;
        uint32                  m_trampolineAddr;
        uint32                  m_returnFromRpcInvokeAddr;
        DynamicModuleList       m_servers;
    };
}

#endif
