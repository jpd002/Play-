#ifndef _IOP_UNKNOWN2_H_
#define _IOP_UNKNOWN2_H_

#include "Iop_Module.h"
#include "Iop_SifMan.h"
#include "zip/ZipArchiveWriter.h"
#include "zip/ZipArchiveReader.h"

namespace Iop
{
	class CUnknown2 : public CModule, public CSifModule
	{
	public:
                            CUnknown2(CSifMan&);
        virtual             ~CUnknown2();

        std::string         GetId() const;
        std::string         GetFunctionName(unsigned int) const;
        void                Invoke(CMIPS&, unsigned int);
        virtual void        Invoke(uint32, uint32*, uint32, uint32*, uint32, uint8*);
        virtual void		SaveState(CZipArchiveWriter&);
        virtual void		LoadState(CZipArchiveReader&);

		enum MODULE_ID
		{
			MODULE_ID = 0x00000030,
		};

	};
};

#endif
