#ifndef _IOP_UNKNOWN2_H_
#define _IOP_UNKNOWN2_H_

#include "Iop_Module.h"
#include "../SIF.h"
#include "zip/ZipArchiveWriter.h"
#include "zip/ZipArchiveReader.h"

namespace Iop
{
	class CUnknown2 : public CModule, public CSifModule
	{
	public:
                            CUnknown2(CSIF&);
        virtual             ~CUnknown2();

        std::string         GetId() const;
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
