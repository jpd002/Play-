#include "NamcoSys147Driver.h"
#include "string_format.h"
#include "AppConfig.h"
#include "PS2VM.h"
#include "PS2VM_Preferences.h"
#include "../ArcadeDefinition.h"

#include "iop/namco_sys147/Iop_NamcoNANDDevice.h"
#include "iop/namco_sys147/Iop_NamcoSys147.h"

void CNamcoSys147Driver::PrepareEnvironment(CPS2VM* virtualMachine, const ARCADE_MACHINE_DEF& def)
{
	auto baseId = def.parent.empty() ? def.id : def.parent;
	fs::path arcadeRomPath = CAppConfig::GetInstance().GetPreferencePath(PREF_PS2_ARCADEROMS_DIRECTORY);

	fs::path nandPath = arcadeRomPath / baseId / def.nandFileName;
	if(!fs::exists(nandPath))
	{
		throw std::runtime_error(string_format("Failed to find '%s' in game's directory.", def.nandFileName.c_str()));
	}

	// clang-format off
	static std::pair<const char*, uint32> mounts[] =
	{
		std::make_pair("atfile0", 0x6000),
		std::make_pair("atfile1", 0x10000),
		std::make_pair("atfile2", 0x20000),
		std::make_pair("atfile3", 0x30000),
		std::make_pair("atfile4", 0x40000),
		std::make_pair("atfile5", 0x50000),
		std::make_pair("atfile6", 0x60000)
	};
	// clang-format on

	auto iopBios = dynamic_cast<CIopBios*>(virtualMachine->m_iop->m_bios.get());
	for(const auto& mount : mounts)
	{
		iopBios->GetIoman()->RegisterDevice(mount.first, std::make_shared<Iop::Namco::CNamcoNANDDevice>(std::make_unique<Framework::CStdStream>(nandPath.string().c_str(), "rb"), mount.second));
	}

	{
		auto sys147Module = std::make_shared<Iop::Namco::CSys147>(*iopBios->GetSifman(), def.id);
		iopBios->RegisterModule(sys147Module);
		iopBios->RegisterHleModuleReplacement("S147LINK", sys147Module);
		virtualMachine->m_pad->InsertListener(sys147Module.get());
	}
}

void CNamcoSys147Driver::Launch(CPS2VM* virtualMachine, const ARCADE_MACHINE_DEF& def)
{
	virtualMachine->m_ee->m_os->BootFromVirtualPath(def.boot.c_str(), {});
}
