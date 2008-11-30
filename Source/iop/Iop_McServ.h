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
        virtual void		Invoke(uint32, uint32*, uint32, uint32*, uint32, uint8*);
//		virtual void		SaveState(Framework::CStream*);
//		virtual void		LoadState(Framework::CStream*);

	private:
		enum MODULE_ID
		{
			MODULE_ID = 0x80000400,
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

		class CPathFinder
		{
		public:
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
										CPathFinder(const boost::filesystem::path&, ENTRY*, unsigned int, const char*);
										~CPathFinder();

			unsigned int				Search();

		private:
			void						SearchRecurse(const boost::filesystem::path&);
			bool						MatchesFilter(const char*);

			ENTRY*						m_pEntry;
			boost::filesystem::path		m_BasePath;
			std::string					m_sFilter;
			unsigned int				m_nIndex;
			unsigned int				m_nMax;

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

		HandleMap			m_Handles;
		static const char*	m_sMcPathPreference[2];
		uint32				m_nNextHandle;
        std::string         m_currentDirectory;
	};

}

#endif
