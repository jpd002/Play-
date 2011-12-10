#ifndef _IOP_DBCMAN320_H_
#define _IOP_DBCMAN320_H_

#include "Iop_Module.h"
#include "Iop_DbcMan.h"
#include "Iop_SifMan.h"
#include "zip/ZipArchiveWriter.h"
#include "zip/ZipArchiveReader.h"

namespace Iop
{
	class CDbcMan320 : public CModule, public CSifModule
	{
	public:
									CDbcMan320(CSifMan&, CDbcMan&);
		virtual						~CDbcMan320();
        std::string                 GetId() const;
        std::string                 GetFunctionName(unsigned int) const;
        void                        Invoke(CMIPS&, unsigned int);
        virtual bool				Invoke(uint32, uint32*, uint32, uint32*, uint32, uint8*);
		virtual void				SaveState(Framework::CZipArchiveWriter&);
		virtual void				LoadState(Framework::CZipArchiveReader&);

	private:
		enum MODULE_ID
		{
			MODULE_ID = 0x80001300
		};

        void                        ReceiveData(uint32*, uint32, uint32*, uint32, uint8*);
        void						GetVersionInformation(uint32*, uint32, uint32*, uint32, uint8*);
		void						InitializeSocket(uint32*, uint32, uint32*, uint32, uint8*);

        CDbcMan&                    m_dbcMan;
	};   
};

#endif
