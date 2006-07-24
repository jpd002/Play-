#ifndef _SAVEEXPORTER_H_
#define _SAVEEXPORTER_H_

#include <iostream>
#include <boost/date_time/posix_time/posix_time.hpp>
#include "Types.h"

class CSaveExporter
{
public:
	static void		ExportPSU(std::ostream&, const char*);
	
private:
#pragma pack(push, 1)

	struct PSUENTRY
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

		uint32	nFlags;
		uint32	nSize;
		TIME	CreationTime;
		uint32	nSector;
		uint32	nChecksum;
		TIME	ModificationTime;
		uint8	Padding[0x20];
		uint8	sName[0x1C0];
	};

#pragma pack(pop)

	static void		PSU_CopyTime(PSUENTRY::TIME*, boost::posix_time::ptime*);

};

#endif
