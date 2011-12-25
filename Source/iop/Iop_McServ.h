#ifndef _IOP_MCSERV_H_
#define _IOP_MCSERV_H_

#include "Iop_Module.h"
#include "Iop_SifMan.h"
#include <string>
#include <map>
#include <boost/filesystem/path.hpp>

namespace Iop
{

	class CMcServ : public CModule, public CSifModule
	{
	public:
							CMcServ(CSifMan&);
		virtual				~CMcServ();
        std::string         GetId() const;
        std::string         GetFunctionName(unsigned int) const;
        void                Invoke(CMIPS&, unsigned int);
        virtual bool		Invoke(uint32, uint32*, uint32, uint32*, uint32, uint8*);

	private:
		enum MODULE_ID
		{
			MODULE_ID = 0x80000400,
		};

		enum OPEN_FLAGS
		{
			O_RDONLY	= 0x00000001,
			O_WRONLY	= 0x00000002,
			O_RDWR		= 0x00000003,
			O_CREAT		= 0x00000200,
		};

        struct CMD
		{
			uint32	nPort;
			uint32	nSlot;
			uint32	nFlags;
			uint32	nMaxEntries;
			uint32	nTableAddress;
			char	sName[0x400];
		};

		struct FILECMD
		{
			uint32	nHandle;
			uint32	nPad[2];
			uint32	nSize;
			uint32	nOffset;
			uint32	nOrigin;
			uint32	nBufferAddress;
			uint32	nParamAddress;
			char	nData[16];
		};

		struct ENTRY
		{
			struct TIME
			{
				uint8	nUnknown;
				uint8	nSecond;
				uint8	nMinute;
				uint8	nHour;
				uint8	nDay;
				uint8	nMonth;
				uint16	nYear;
			};

			TIME	CreationTime;
			TIME	ModificationTime;
			uint32	nSize;
			uint16	nAttributes;
			uint16	nReserved0;
			uint32	nReserved1[2];
			uint8	sName[0x20];
		};

		class CPathFinder
		{
		public:
										CPathFinder();
			virtual                     ~CPathFinder();

            void                        Reset();
			void        				Search(const boost::filesystem::path&, const char*);
            unsigned int                Read(ENTRY*, unsigned int);

		private:
            typedef std::vector<ENTRY> EntryList;
            typedef bool (CPathFinder::*StringMatcher)(const char*);

			void						SearchRecurse(const boost::filesystem::path&);
			bool						StarFilterMatcher(const char*);
            bool                        QuestionMarkFilterMatcher(const char*);

            EntryList                   m_entries;

//          ENTRY*						m_pEntry;
			boost::filesystem::path		m_BasePath;
			std::string					m_sFilter;
			unsigned int				m_nIndex;
//			unsigned int				m_nMax;
            StringMatcher               m_matcher;
		};

		void				GetInfo(uint32*, uint32, uint32*, uint32, uint8*);
		void				Open(uint32*, uint32, uint32*, uint32, uint8*);
		void				Close(uint32*, uint32, uint32*, uint32, uint8*);
        void                Seek(uint32*, uint32, uint32*, uint32, uint8*);
		void				Read(uint32*, uint32, uint32*, uint32, uint8*);
        void                Write(uint32*, uint32, uint32*, uint32, uint8*);
        void                ChDir(uint32*, uint32, uint32*, uint32, uint8*);
		void				GetDir(uint32*, uint32, uint32*, uint32, uint8*);
		void				GetVersionInformation(uint32*, uint32, uint32*, uint32, uint8*);

		uint32				GenerateHandle();
        FILE*               GetFileFromHandle(uint32);

		typedef std::map<uint32, FILE*> HandleMap;

		HandleMap					m_Handles;
		static const char*			m_sMcPathPreference[2];
		uint32						m_nNextHandle;
		boost::filesystem::path		m_currentDirectory;
        CPathFinder					m_pathFinder;
	};

}

#endif
