#ifndef _IOP_CDVDFSV_H_
#define _IOP_CDVDFSV_H_

#include "IOP_Module.h"

namespace IOP
{

	class CCdvdfsv : public CModule
	{
	public:
						CCdvdfsv(uint32);
		virtual			~CCdvdfsv();
		virtual void	Invoke(uint32, void*, uint32, void*, uint32);
		virtual void	SaveState(Framework::CStream*);
		virtual void	LoadState(Framework::CStream*);

		enum MODULE_ID
		{
			MODULE_ID_1 = 0x80000592,
			MODULE_ID_2 = 0x80000593,
			MODULE_ID_3 = 0x80000594,
			MODULE_ID_4 = 0x80000595,
			MODULE_ID_5 = 0x80000596,
			MODULE_ID_6 = 0x80000597,
		};

	private:
		void			Invoke592(uint32, void*, uint32, void*, uint32);
		void			Invoke595(uint32, void*, uint32, void*, uint32);
		void			Invoke597(uint32, void*, uint32, void*, uint32);

		//Methods
		void			Read(void*, uint32, void*, uint32);
		void			StreamCmd(void*, uint32, void*, uint32);
		void			SearchFile(void*, uint32, void*, uint32);

		uint32			m_nID;
		static uint32	m_nStreamPos;
	};

}

#endif
