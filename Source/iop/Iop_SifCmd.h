#pragma once

#include "Iop_Module.h"
#include "Iop_SifMan.h"
#include "Iop_SifDynamic.h"
#include "Iop_Sysmem.h"
#include "zip/ZipArchiveWriter.h"
#include "zip/ZipArchiveReader.h"

class CIopBios;

namespace Iop
{
	class CSifCmd : public CModule
	{
	public:
								CSifCmd(CIopBios&, CSifMan&, CSysmem&, uint8*);
		virtual					~CSifCmd();

		std::string				GetId() const override;
		std::string				GetFunctionName(unsigned int) const override;
		void					Invoke(CMIPS&, unsigned int) override;

		void					ProcessInvocation(uint32, uint32, uint32*, uint32);

		void					LoadState(Framework::CZipArchiveReader&);
		void					SaveState(Framework::CZipArchiveWriter&);

	private:
		typedef std::list<CSifDynamic*> DynamicModuleList;

		struct SIFRPCQUEUEDATA
		{
			uint32		threadId;
			uint32		active;
			uint32		serverDataLink;    //Set when there's a pending request
			uint32		serverDataStart;   //Set when a RPC server has been registered on the queue
			uint32		serverDataEnd;
			uint32		queueNext;
		};

		struct SIFRPCSERVERDATA
		{
			uint32		serverId;

			uint32		function;
			uint32		buffer;
			uint32		size;

			uint32		cfunction;
			uint32		cbuffer;
			uint32		csize;

			uint32		rsize;
			uint32		rid;

			uint32		queueAddr;
		};
		static_assert(sizeof(SIFRPCSERVERDATA) <= 0x44, "Size of SIFRPCSERVERDATA must be less or equal to 68 bytes.");

		struct SIFCMDDATA
		{
			uint32		sifCmdHandler;
			uint32		data;
			uint32		gp;
		};
		
		enum
		{
			MAX_SYSTEM_COMMAND = 0x20
		};

		struct MODULEDATA
		{
			uint8      trampoline[0x800];
			uint8      sendCmdExtraStruct[0x10];
			SIFCMDDATA sysCmdBuffer[MAX_SYSTEM_COMMAND];
		};

		void					ClearServers();
		void					BuildExportTable();

		void					ProcessCustomCommand(uint32);
		void					ProcessRpcRequestEnd(uint32);
		void					ProcessDynamicCommand(uint32);

		uint32					SifSetCmdBuffer(uint32, uint32);
		void					SifAddCmdHandler(uint32, uint32, uint32);
		uint32					SifSendCmd(uint32, uint32, uint32, uint32, uint32, uint32);
		uint32					SifBindRpc(uint32, uint32, uint32);
		void					SifCallRpc(CMIPS&);
		void					SifRegisterRpc(CMIPS&);
		uint32					SifCheckStatRpc(uint32);
		void					SifSetRpcQueue(uint32, uint32);
		uint32					SifGetNextRequest(uint32);
		void					SifExecRequest(CMIPS&);
		void					SifRpcLoop(CMIPS&);
		uint32					SifGetOtherData(uint32, uint32, uint32, uint32, uint32);
		void					FinishExecRequest(uint32, uint32);
		void					SleepThread();

		uint32					m_usrCmdBuffer = 0;
		uint32					m_usrCmdBufferLen = 0;
		uint32					m_sysCmdBuffer = 0;

		CIopBios&				m_bios;
		CSifMan&				m_sifMan;
		CSysmem&				m_sysMem;
		uint8*					m_ram;
		uint32					m_memoryBufferAddr;
		uint32					m_trampolineAddr;
		uint32					m_sendCmdExtraStructAddr;
		uint32					m_sifRpcLoopAddr = 0;
		uint32					m_sifExecRequestAddr = 0;
		DynamicModuleList		m_servers;
	};

	typedef std::shared_ptr<CSifCmd> SifCmdPtr;
}
