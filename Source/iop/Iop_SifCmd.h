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

		struct SIFRPCDATAQUEUE
		{
			uint32		threadId;
			uint32		active;
			uint32		serverDataLink;
			uint32		serverDataStart;
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

			uint32		queueAddr;
		};

		// m_cmdBuffer is an array of these structures.
		struct SIFCMDDATA
		{
			uint32		sifCmdHandler;
			uint32		data;
			uint32		gp;
		};

		void					ClearServers();
		void					BuildExportTable();

		void					ProcessCustomCommand(uint32);

		uint32					SifSendCmd(uint32, uint32, uint32, uint32, uint32, uint32);
		uint32					SifBindRpc(uint32, uint32, uint32);
		void					SifCallRpc(CMIPS&);
		void					SifRegisterRpc(CMIPS&);
		uint32					SifCheckStatRpc(uint32);
		void					SifSetRpcQueue(uint32, uint32);
		void					SifRpcLoop(uint32);
		uint32					SifGetOtherData(uint32, uint32, uint32, uint32, uint32);
		void					ReturnFromRpcInvoke(CMIPS&);
		uint32					SifSetCmdBuffer(uint32 pData, uint32 len);
		void					SifAddCmdHandler(uint32 pos, uint32 handler, uint32 data);

		uint32					m_cmdBuffer = 0;
		uint32					m_cmdBufferLen = 0;

		CIopBios&				m_bios;
		CSifMan&				m_sifMan;
		CSysmem&				m_sysMem;
		uint8*					m_ram;
		uint32					m_memoryBufferAddr;
		uint32					m_trampolineAddr;
		uint32					m_sendCmdExtraStructAddr;
		uint32					m_returnFromRpcInvokeAddr;
		DynamicModuleList		m_servers;
	};
}
