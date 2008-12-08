#ifndef _IOP_UNKNOWN_H_
#define _IOP_UNKNOWN_H_

#include "Iop_Module.h"
#include "Iop_SifMan.h"
#include "zip/ZipArchiveWriter.h"
#include "zip/ZipArchiveReader.h"

namespace Iop
{
	class CUnknown : public CModule, public CSifModule
	{
	public:
                            CUnknown(CSifMan&);
        virtual             ~CUnknown();

        std::string         GetId() const;
        std::string         GetFunctionName(unsigned int) const;
        void                Invoke(CMIPS&, unsigned int);
        virtual bool        Invoke(uint32, uint32*, uint32, uint32*, uint32, uint8*);
        virtual void		SaveState(CZipArchiveWriter&);
        virtual void		LoadState(CZipArchiveReader&);

		enum MODULE_ID
		{
			MODULE_ID = 0x00012345,
		};
	};
};

#endif
