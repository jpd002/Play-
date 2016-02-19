#pragma once

#include "Iop_FileIo.h"
#include "Iop_Ioman.h"

namespace Iop
{
	class CFileIoHandler2300 : public CFileIo::CHandler
	{
	public:
						CFileIoHandler2300(CIoman*, CSifMan&);

		virtual void	Invoke(uint32, uint32*, uint32, uint32*, uint32, uint8*) override;

	private:
		struct COMMANDHEADER
		{
			uint32			semaphoreId;
			uint32			resultPtr;
			uint32			resultSize;
		};

		struct OPENCOMMAND
		{
			COMMANDHEADER	header;
			uint32			flags;
			uint32			somePtr1;
			char			fileName[256];
		};

		struct CLOSECOMMAND
		{
			COMMANDHEADER	header;
			uint32			fd;
		};

		struct READCOMMAND
		{
			COMMANDHEADER	header;
			uint32			fd;
			uint32			buffer;
			uint32			size;
		};

		struct SEEKCOMMAND
		{
			COMMANDHEADER	header;
			uint32			fd;
			uint32			offset;
			uint32			whence;
		};

		struct DOPENCOMMAND
		{
			COMMANDHEADER	header;
			char			dirName[256];
		};

		struct GETSTATCOMMAND
		{
			COMMANDHEADER	header;
			uint32			statBuffer;
			char			fileName[256];
		};

		struct ACTIVATECOMMAND
		{
			COMMANDHEADER	header;
			char			device[256];
		};

		struct REPLYHEADER
		{
			uint32			semaphoreId;
			uint32			commandId;
			uint32			resultPtr;
			uint32			resultSize;
		};

		struct OPENREPLY
		{
			REPLYHEADER		header;
			uint32			result;
			uint32			unknown2;
			uint32			unknown3;
			uint32			unknown4;
		};

		struct CLOSEREPLY
		{
			REPLYHEADER		header;
			uint32			result;
			uint32			unknown2;
			uint32			unknown3;
			uint32			unknown4;
		};

		struct READREPLY
		{
			REPLYHEADER		header;
			uint32			result;
			uint32			unknown2;
			uint32			unknown3;
			uint32			unknown4;
		};

		struct SEEKREPLY
		{
			REPLYHEADER		header;
			uint32			result;
			uint32			unknown2;
			uint32			unknown3;
			uint32			unknown4;
		};

		struct DOPENREPLY
		{
			REPLYHEADER		header;
			uint32			result;
			uint32			unknown2;
			uint32			unknown3;
			uint32			unknown4;
		};

		struct GETSTATREPLY
		{
			REPLYHEADER		header;
			uint32			result;
			uint32			dstPtr;
			CIoman::STAT	stat;
		};

		struct ACTIVATEREPLY
		{
			REPLYHEADER		header;
			uint32			result;
			uint32			unknown2;
			uint32			unknown3;
			uint32			unknown4;
		};

		uint32			InvokeOpen(uint32*, uint32, uint32*, uint32, uint8*);
		uint32			InvokeClose(uint32*, uint32, uint32*, uint32, uint8*);
		uint32			InvokeRead(uint32*, uint32, uint32*, uint32, uint8*);
		uint32			InvokeSeek(uint32*, uint32, uint32*, uint32, uint8*);
		uint32			InvokeDopen(uint32*, uint32, uint32*, uint32, uint8*);
		uint32			InvokeGetStat(uint32*, uint32, uint32*, uint32, uint8*);
		uint32			InvokeActivate(uint32*, uint32, uint32*, uint32, uint8*);

		void			CopyHeader(REPLYHEADER&, const COMMANDHEADER&);
		void			SendSifReply();

		uint32			m_resultPtr[2];
		CSifMan&		m_sifMan;
	};
};
