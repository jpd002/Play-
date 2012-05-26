#ifndef _PSP_PSFBIOS_H_
#define _PSP_PSFBIOS_H_

#include "PspBios.h"
#include "Psp_PsfDevice.h"

namespace Psp
{
	class CPsfBios : public CBiosDebugInfoProvider
	{
	public:
									CPsfBios(CMIPS&, uint8*, uint32);
		virtual						~CPsfBios();

		void						AppendArchive(const CPsfBase&);
		void						Start();
		void						Reset();

		CSasCore*					GetSasCore();
		CAudio*						GetAudio();

		void						HandleException();
		void						HandleInterrupt();
		void						CountTicks(uint32);

		bool						IsIdle();

		BiosDebugModuleInfoArray	GetModuleInfos() const;
		BiosDebugThreadInfoArray	GetThreadInfos() const;

#ifdef DEBUGGER_INCLUDED
		void						LoadDebugTags(Framework::Xml::CNode*);
		void						SaveDebugTags(Framework::Xml::CNode*);
#endif

	private:
		CBios						m_bios;
		PsfDevicePtr				m_psfDevice;
	};
}

#endif
