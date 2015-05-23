#pragma once

#include <string>
#include <map>
#include <regex>
#include <boost/filesystem.hpp>
#include "StdStream.h"
#include "Iop_Module.h"
#include "Iop_SifMan.h"

namespace Iop
{

	class CMcServ : public CModule, public CSifModule
	{
	public:
		struct CMD
		{
			uint32	port;
			uint32	slot;
			uint32	flags;
			int32	maxEntries;
			uint32	tableAddress;
			char	name[0x400];
		};
		static_assert(sizeof(CMD) == 0x414, "Size of CMD structure must be 0x414 bytes.");

		struct ENTRY
		{
			struct TIME
			{
				uint8	unknown;
				uint8	second;
				uint8	minute;
				uint8	hour;
				uint8	day;
				uint8	month;
				uint16	year;
			};

			TIME	creationTime;
			TIME	modificationTime;
			uint32	size;
			uint16	attributes;
			uint16	reserved0;
			uint32	reserved1[2];
			uint8	name[0x20];
		};

							CMcServ(CSifMan&);
		virtual				~CMcServ();

		static const char*	GetMcPathPreference(unsigned int);

		std::string			GetId() const override;
		std::string			GetFunctionName(unsigned int) const override;
		void				Invoke(CMIPS&, unsigned int) override;
		bool				Invoke(uint32, uint32*, uint32, uint32*, uint32, uint8*) override;

	private:
		enum MODULE_ID
		{
			MODULE_ID = 0x80000400,
		};

		enum RETURN_CODES
		{
			RET_OK					= 0,
			RET_NO_ENTRY			= -4,
			RET_PERMISSION_DENIED	= -5
		};

		enum OPEN_FLAGS
		{
			OPEN_FLAG_RDONLY	= 0x00000001,
			OPEN_FLAG_WRONLY	= 0x00000002,
			OPEN_FLAG_RDWR		= 0x00000003,
			OPEN_FLAG_CREAT		= 0x00000200,
			OPEN_FLAG_TRUNC		= 0x00000400,
		};

		enum
		{
			MAX_FILES = 5
		};

		struct FILECMD
		{
			uint32	handle;
			uint32	pad[2];
			uint32	size;
			uint32	offset;
			uint32	origin;
			uint32	bufferAddress;
			uint32	paramAddress;
			char	data[16];
		};

		class CPathFinder
		{
		public:
										CPathFinder();
			virtual						~CPathFinder();

			void						Reset();
			void						Search(const boost::filesystem::path&, const char*);
			unsigned int				Read(ENTRY*, unsigned int);

		private:
			typedef std::vector<ENTRY> EntryList;

			void						SearchRecurse(const boost::filesystem::path&);

			EntryList					m_entries;
			boost::filesystem::path		m_basePath;
			std::regex					m_filterExp;
			unsigned int				m_index;
		};

		void				GetInfo(uint32*, uint32, uint32*, uint32, uint8*);
		void				Open(uint32*, uint32, uint32*, uint32, uint8*);
		void				Close(uint32*, uint32, uint32*, uint32, uint8*);
		void				Seek(uint32*, uint32, uint32*, uint32, uint8*);
		void				Read(uint32*, uint32, uint32*, uint32, uint8*);
		void				Write(uint32*, uint32, uint32*, uint32, uint8*);
		void				Flush(uint32*, uint32, uint32*, uint32, uint8*);
		void				ChDir(uint32*, uint32, uint32*, uint32, uint8*);
		void				GetDir(uint32*, uint32, uint32*, uint32, uint8*);
		void				Delete(uint32*, uint32, uint32*, uint32, uint8*);
		void				GetVersionInformation(uint32*, uint32, uint32*, uint32, uint8*);

		uint32						GenerateHandle();
		Framework::CStdStream*		GetFileFromHandle(uint32);
		boost::filesystem::path		GetAbsoluteFilePath(unsigned int, unsigned int, const char*) const;

		Framework::CStdStream		m_files[MAX_FILES];
		static const char*			m_mcPathPreference[2];
		boost::filesystem::path		m_currentDirectory;
		CPathFinder					m_pathFinder;
	};

}
