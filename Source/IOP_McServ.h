#ifndef _IOP_MCSERV_H_
#define _IOP_MCSERV_H_

#include "IOP_Module.h"
#include <string>
#include <boost/filesystem/path.hpp>

namespace IOP
{

	class CMcServ : public CModule
	{
	public:
							CMcServ();
		virtual				~CMcServ();
		virtual void		Invoke(uint32, void*, uint32, void*, uint32);
		virtual void		SaveState(Framework::CStream*);
		virtual void		LoadState(Framework::CStream*);

		enum MODULE_ID
		{
			MODULE_ID = 0x80000400,
		};

	private:
		void				GetInfo(void*, uint32, void*, uint32);
		void				GetDir(void*, uint32, void*, uint32);
		void				GetVersionInformation(void*, uint32, void*, uint32);
		void				Log(const char*, ...);

		static const char*	m_sMcPathPreference[2];

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
	};

}

#endif
