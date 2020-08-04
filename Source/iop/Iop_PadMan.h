#pragma once

#include "Iop_Module.h"
#include "Iop_SifMan.h"
#include "Iop_SifModuleProvider.h"
#include "../PadListener.h"
#include <functional>
#include "zip/ZipArchiveWriter.h"
#include "zip/ZipArchiveReader.h"

//#define USE_EX

namespace Iop
{
	class CPadMan : public CModule, public CPadListener, public CSifModule, public CSifModuleProvider
	{
	public:
		CPadMan();

		std::string GetId() const override;
		std::string GetFunctionName(unsigned int) const override;

		void RegisterSifModules(CSifMan&) override;

		void Invoke(CMIPS&, unsigned int) override;
		bool Invoke(uint32, uint32*, uint32, uint32*, uint32, uint8*) override;
		void SaveState(Framework::CZipArchiveWriter&);
		void LoadState(Framework::CZipArchiveReader&);
		void SetButtonState(unsigned int, PS2::CControllerInfo::BUTTON, bool, uint8*) override;
		void SetAxisState(unsigned int, PS2::CControllerInfo::BUTTON, uint8, uint8*) override;

		enum MODULE_ID
		{
			MODULE_ID_1 = 0x80000100,
			MODULE_ID_2 = 0x80000101,
			MODULE_ID_3 = 0x8000010F,
			MODULE_ID_4 = 0x8000011F,
		};

	private:
		struct PADDATA
		{
			uint32 nFrame;
			uint8 nState;
			uint8 nReqState;
			uint8 nOk;
			uint8 nReserved0;
			uint8 nData[32];
			uint32 nLength;
			uint32 nReserved1[5];
		};
		static_assert(sizeof(PADDATA) == 0x40, "Size of PADDATA struct must be 64 bytes.");

		struct PADDATA80
		{
			uint32 nFrame;
			uint8 nState;
			uint8 nReqState;
			uint8 nOk;
			uint8 nReserved0;
			uint8 nData[32];
			uint32 nLength;
			uint32 nReserved1[5];
			uint8 nReserved2[0x40];
		};
		static_assert(sizeof(PADDATA80) == 0x80, "Size of PADDATA80 struct must be 128 bytes.");

		struct PADDATAEX
		{
			uint8 nData[32];        //+0x00
			uint32 nReserved0;      //+0x20
			uint32 nReserved1;      //+0x24
			uint32 nReserved2;      //+0x28
			uint32 nReserved3;      //+0x2C
			uint8 nReserved4[0x20]; //+0x30
			uint16 nModeTable[4];   //+0x50
			uint32 nFrame;          //+0x58
			uint32 nReserved5;      //+0x5C
			uint32 nLength;         //+0x60
			uint8 nModeOk;          //+0x64
			uint8 nModeCurId;       //+0x65
			uint8 nReserved6[2];    //+0x66
			uint8 nNrOfModes;       //+0x68
			uint8 nModeCurOffset;   //+0x69
			uint8 nReserved7[0x6];  //+0x6A
			uint8 nState;           //+0x70
			uint8 nReqState;        //+0x71
			uint8 nOk;              //+0x72
			uint8 nReserved8;       //+0x73
			uint32 nReserved9[3];   //+0x74
		};
		static_assert(sizeof(PADDATAEX) == 0x80, "Size of PADDATAEX struct must be 128 bytes.");

		class CPadDataInterface
		{
		public:
			virtual ~CPadDataInterface() = default;
			virtual void SetData(unsigned int, uint8) = 0;
			virtual uint8 GetData(unsigned int) = 0;
			virtual void SetFrame(unsigned int) = 0;
			virtual void SetState(unsigned int) = 0;
			virtual void SetReqState(unsigned int) = 0;
			virtual void SetLength(unsigned int) = 0;
			virtual void SetOk(unsigned int) = 0;
			virtual void SetModeCurId(unsigned int) = 0;
			virtual void SetModeCurOffset(unsigned int) = 0;
			virtual void SetModeTable(unsigned int, unsigned int) = 0;
			virtual void SetNumberOfModes(unsigned int) = 0;
		};

		template <typename T>
		class CPadDataHandler : public CPadDataInterface
		{
		public:
			CPadDataHandler(void* pPtr)
			{
				m_pPadData = reinterpret_cast<T*>(pPtr);
			}

			uint8 GetData(unsigned int nIndex) override
			{
				return m_pPadData->nData[nIndex];
			}

			void SetData(unsigned int nIndex, uint8 nValue) override
			{
				m_pPadData->nData[nIndex] = nValue;
			}

			void SetFrame(unsigned int nValue) override
			{
				m_pPadData->nFrame = nValue;
			}

			void SetState(unsigned int nValue) override
			{
				m_pPadData->nState = nValue;
			}

			void SetReqState(unsigned int nValue) override
			{
				m_pPadData->nReqState = nValue;
			}

			void SetLength(unsigned int nValue) override
			{
				m_pPadData->nLength = nValue;
			}

			void SetOk(unsigned int nValue) override
			{
				m_pPadData->nOk = nValue;
			}

			void SetModeCurId(unsigned int nValue) override
			{
				m_pPadData->nModeCurId = nValue;
			}

			void SetModeCurOffset(unsigned int nValue) override
			{
				m_pPadData->nModeCurOffset = nValue;
			}

			void SetModeTable(unsigned int nIndex, unsigned int nValue) override
			{
				m_pPadData->nModeTable[nIndex] = nValue;
			}

			void SetNumberOfModes(unsigned int number) override
			{
				m_pPadData->nNrOfModes = number;
			}

		private:
			T* m_pPadData;
		};

		typedef std::function<void(CPadDataInterface*)> PadDataFunction;

		enum PAD_DATA_TYPE
		{
			PAD_DATA_STD,
			PAD_DATA_STD80,
			PAD_DATA_EX
		};

		PAD_DATA_TYPE m_nPadDataType;
		uint32 m_nPadDataAddress;
		uint32 m_nPadDataAddress1;

		void Open(uint32*, uint32, uint32*, uint32, uint8*);
		void Close(uint32*, uint32, uint32*, uint32, uint8*);
		void SetActuatorAlign(uint32*, uint32, uint32*, uint32, uint8*);
		void Init(uint32*, uint32, uint32*, uint32, uint8*);
		void GetModuleVersion(uint32*, uint32, uint32*, uint32, uint8*);
		void ExecutePadDataFunction(const PadDataFunction&, void*, size_t);

		static PAD_DATA_TYPE GetDataType(uint8*);

		static void PDF_InitializeStruct0(CPadDataInterface*);
		static void PDF_InitializeStruct1(CPadDataInterface*);
		static void PDF_SetButtonState(CPadDataInterface*, PS2::CControllerInfo::BUTTON, bool);
		static void PDF_SetAxisState(CPadDataInterface*, PS2::CControllerInfo::BUTTON, uint8);
	};

	typedef std::shared_ptr<CPadMan> PadManPtr;

	template <>
	inline void CPadMan::CPadDataHandler<CPadMan::PADDATA>::SetModeCurId(unsigned int)
	{
	}
	template <>
	inline void CPadMan::CPadDataHandler<CPadMan::PADDATA>::SetModeCurOffset(unsigned int)
	{
	}
	template <>
	inline void CPadMan::CPadDataHandler<CPadMan::PADDATA>::SetModeTable(unsigned int, unsigned int)
	{
	}
	template <>
	inline void CPadMan::CPadDataHandler<CPadMan::PADDATA>::SetNumberOfModes(unsigned int)
	{
	}

	template <>
	inline void CPadMan::CPadDataHandler<CPadMan::PADDATA80>::SetModeCurId(unsigned int)
	{
	}
	template <>
	inline void CPadMan::CPadDataHandler<CPadMan::PADDATA80>::SetModeCurOffset(unsigned int)
	{
	}
	template <>
	inline void CPadMan::CPadDataHandler<CPadMan::PADDATA80>::SetModeTable(unsigned int, unsigned int)
	{
	}
	template <>
	inline void CPadMan::CPadDataHandler<CPadMan::PADDATA80>::SetNumberOfModes(unsigned int)
	{
	}
}
