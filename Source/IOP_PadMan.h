#ifndef _IOP_PADMAN_H_
#define _IOP_PADMAN_H_

#include "IOP_Module.h"
#include "PadListener.h"

//#define USE_EX

namespace IOP
{
	class CPadMan : public CModule, public CPadListener
	{
	public:
							CPadMan();
		virtual void		Invoke(uint32, void*, uint32, void*, uint32);
		virtual void		SaveState(Framework::CStream*);
		virtual void		LoadState(Framework::CStream*);
		virtual void		SetButtonState(unsigned int, CPadListener::BUTTON, bool);

		enum MODULE_ID
		{
			MODULE_ID_1 = 0x80000100,
			MODULE_ID_2 = 0x80000101,
			MODULE_ID_3 = 0x8000010F,
			MODULE_ID_4 = 0x0000011F,
		};

	private:
		class CPadDataInterface
		{
		public:
			virtual				~CPadDataInterface() {}
			virtual void 		SetData(unsigned int, uint8) = 0;
			virtual uint8		GetData(unsigned int) = 0;
			virtual size_t		GetSize() = 0;
			virtual void		SetFrame(unsigned int) = 0;
		};

		template <typename T> class CPadDataHandler : public CPadDataInterface
		{
		public:
			CPadDataHandler(void* pPtr)
			{
				m_pPadData = reinterpret_cast<T*>(pPtr);
			}

			virtual ~CPadDataHandler()
			{

			}

			virtual uint8 GetData(unsigned int nIndex)
			{
				return m_pPadData->nData[nIndex];
			}

			virtual void SetData(unsigned int nIndex, uint8 nValue)
			{
				m_pPadData->nData[nIndex] = nValue;
			}

			virtual size_t GetSize()
			{
				return sizeof(T);
			}

			virtual void SetFrame(unsigned int nValue)
			{
				m_pPadData->nFrame = nValue;
			}

		private:
			T*		m_pPadData;
		};

		struct PADDATAEX
		{
			uint8			nData[32];
			uint32			nReserved1[4];
			uint8			nActuator[32];
			uint16			nModeTable[4];
			uint32			nFrame;
			uint32			nReserved2;
			uint32			nLength;
			uint8			nModeOk;
			uint8			nModeCurId;
			uint8			nReserved3[2];
			uint8			nNrOfModes;
			uint8			nModeCurOffset;
			uint8			nActuatorCount;
			uint8			nReserved4[5];
			uint8			nState;
			uint8			nReqState;
			uint8			nOk;
			uint8			nReserved5[13];
		};

		struct PADDATA
		{
			uint32			nFrame;
			uint8			nState;
			uint8			nReqState;
			uint8			nOk;
			uint8			nReserved0;
			uint8			nData[32];
			uint32			nLength;
			uint32			nReserved1[5];
		};

		PADDATA*			m_pPad;

		void				Open(void*, uint32, void*, uint32);
		void				SetActuatorAlign(void*, uint32, void*, uint32);
		void				Init(void*, uint32, void*, uint32);
		void				GetModuleVersion(void*, uint32, void*, uint32);

		CPadDataInterface&	CreatePadDataHandler();

		static void			Log(const char*, ...);
	};
}

#endif
