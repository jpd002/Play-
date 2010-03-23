#ifndef _PSP_SASCORE_H_
#define _PSP_SASCORE_H_

#include "PspModule.h"

namespace Psp
{
	class CSasCore : public CModule
	{
	public:
						CSasCore();
		virtual			~CSasCore();

		void			Invoke(uint32, CMIPS&);
		std::string		GetName() const;

	private:
		uint32			Init(uint32, uint32, uint32, uint32, uint32);
		uint32			Core(uint32, uint32);
		uint32			SetVoice(uint32, uint32, uint32, uint32, uint32);
		uint32			SetPitch(uint32, uint32, uint32);
		uint32			SetVolume(uint32, uint32, uint32, uint32, uint32, uint32);
		uint32			SetSimpleADSR(uint32, uint32, uint32, uint32);
		uint32			SetKeyOn(uint32, uint32);
		uint32			SetKeyOff(uint32, uint32);
		uint32			GetPauseFlag(uint32);
		uint32			GetEndFlag(uint32);
		uint32			GetAllEnvelope(uint32, uint32);
	};
}

#endif
