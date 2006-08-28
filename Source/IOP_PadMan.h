#ifndef _IOP_PADMAN_H_
#define _IOP_PADMAN_H_

#include "IOP_Module.h"
#include "PadListener.h"
#include <boost/function.hpp>

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

		class CPadDataInterface
		{
		public:
			virtual				~CPadDataInterface() {}
			virtual void 		SetData(unsigned int, uint8) = 0;
			virtual uint8		GetData(unsigned int) = 0;
			virtual void		SetFrame(unsigned int) = 0;
			virtual void		SetState(unsigned int) = 0;
			virtual void		SetReqState(unsigned int) = 0;
			virtual void		SetLength(unsigned int) = 0;
			virtual void		SetOk(unsigned int) = 0;
			virtual void		SetModeCurId(unsigned int) = 0;
			virtual void		SetModeCurOffset(unsigned int) = 0;
			virtual void		SetModeTable(unsigned int, unsigned int) = 0;
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

			virtual void SetFrame(unsigned int nValue)
			{
				m_pPadData->nFrame = nValue;
			}

			virtual void SetState(unsigned int nValue)
			{
				m_pPadData->nState = nValue;
			}

			virtual void SetReqState(unsigned int nValue)
			{
				m_pPadData->nReqState = nValue;
			}

			virtual void SetLength(unsigned int nValue)
			{
				m_pPadData->nLength = nValue;
			}

			virtual void SetOk(unsigned int nValue)
			{
				m_pPadData->nOk = nValue;
			}

			virtual void SetModeCurId(unsigned int nValue)
			{
				m_pPadData->nModeCurId = nValue;
			}

			virtual void SetModeCurOffset(unsigned int nValue)
			{
				m_pPadData->nModeCurOffset = nValue;
			}

			virtual void SetModeTable(unsigned int nIndex, unsigned int nValue)
			{
				m_pPadData->nModeTable[nIndex] = nValue;
			}

		private:
			T*		m_pPadData;
		};

		typedef boost::function< void (CPadDataInterface*) > PadDataFunction;

		PADDATA*			m_pPad;

		unsigned int		m_nPadDataType;
		uint32				m_nPadDataAddress;

		void				Open(void*, uint32, void*, uint32);
		void				SetActuatorAlign(void*, uint32, void*, uint32);
		void				Init(void*, uint32, void*, uint32);
		void				GetModuleVersion(void*, uint32, void*, uint32);
		void				ExecutePadDataFunction(PadDataFunction, void*, size_t);

		static void			PDF_InitializeStruct0(CPadDataInterface*);
		static void			PDF_InitializeStruct1(CPadDataInterface*);
		static void			PDF_SetButtonState(CPadDataInterface*, BUTTON, bool);

		static void			Log(const char*, ...);
	};

	template <> virtual void CPadMan::CPadDataHandler<CPadMan::PADDATA>::SetModeCurId(unsigned int)
	{

	}

	template <> virtual void CPadMan::CPadDataHandler<CPadMan::PADDATA>::SetModeCurOffset(unsigned int)
	{

	}

	template <> virtual void CPadMan::CPadDataHandler<CPadMan::PADDATA>::SetModeTable(unsigned int, unsigned int)
	{
		
	}
}

#endif
