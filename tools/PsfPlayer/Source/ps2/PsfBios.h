#ifndef _PS2_PSFBIOS_H_
#define _PS2_PSFBIOS_H_

#include "iop/Iop_BiosBase.h"
#include "iop/IopBios.h"
#include "Ps2_PsfDevice.h"

namespace PS2
{
	class CPsfBios : public Iop::CBiosBase
	{
	public:
									CPsfBios(CMIPS&, uint8*, uint32, uint8*);
		virtual						~CPsfBios() = default;
		void						HandleException() override;
		void						HandleInterrupt() override;
		void						CountTicks(uint32) override;

		void						SaveState(Framework::CZipArchiveWriter&) override;
		void						LoadState(Framework::CZipArchiveReader&) override;

		void						NotifyVBlankStart() override;
		void						NotifyVBlankEnd() override;

		bool						IsIdle() override;

#ifdef DEBUGGER_INCLUDED
		void						LoadDebugTags(Framework::Xml::CNode*) override;
		void						SaveDebugTags(Framework::Xml::CNode*) override;

		BiosDebugModuleInfoArray	GetModulesDebugInfo() const override;
		BiosDebugThreadInfoArray	GetThreadsDebugInfo() const override;
#endif

		void						AppendArchive(const CPsfBase&);
		void						Start();

	private:
		Iop::CIoman::DevicePtr		m_psfDevice;
		CIopBios					m_bios;
	};
}

#endif
