#ifndef _IOP_DBCMAN320_H_
#define _IOP_DBCMAN320_H_

#include "Iop_Module.h"
#include "Iop_DbcMan.h"
#include "../SIF.h"
#include "zip/ZipArchiveWriter.h"
#include "zip/ZipArchiveReader.h"

namespace Iop
{
	class CDbcMan320 : public CModule, public CSifModule
	{
	public:
									CDbcMan320(CSIF&, CDbcMan&);
		virtual						~CDbcMan320();
        std::string                 GetId() const;
        void                        Invoke(CMIPS&, unsigned int);
        virtual void				Invoke(uint32, uint32*, uint32, uint32*, uint32, uint8*);
		virtual void				SaveState(CZipArchiveWriter&);
		virtual void				LoadState(CZipArchiveReader&);

	private:
		enum MODULE_ID
		{
			MODULE_ID = 0x80001300
		};

		void						GetVersionInformation(uint32*, uint32, uint32*, uint32, uint8*);
        CDbcMan&                    m_dbcMan;
	};   
};

#endif
