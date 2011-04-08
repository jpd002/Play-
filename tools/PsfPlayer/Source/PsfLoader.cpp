#include "PsfLoader.h"
#include "StdStream.h"

#include "psx/PsxBios.h"
#include "ps2/PsfBios.h"
#include "Iop_PsfSubSystem.h"
#include "Ps2Const.h"

#include "psp/Psp_PsfSubSystem.h"

#include <boost/filesystem.hpp>
#include <boost/lexical_cast.hpp>

using namespace Framework;
namespace filesystem = boost::filesystem;

void CPsfLoader::LoadPsf(CPsfVm& virtualMachine, const char* pathString, const char* archivePath, CPsfBase::TagMap* tags)
{
	boost::scoped_ptr<CPsfStreamProvider> streamProvider(CreatePsfStreamProvider(archivePath));

	size_t pathLength = strlen(pathString);
	if(pathString[pathLength - 1] == '2')
	{
		LoadPs2(virtualMachine, pathString, streamProvider.get(), tags);
	}
	else if(pathString[pathLength - 1] == 'p')
	{
		LoadPsp(virtualMachine, pathString, streamProvider.get(), tags);
	}
	else
	{
		LoadPsx(virtualMachine, pathString, streamProvider.get(), tags);
	}
}

void CPsfLoader::LoadPsx(CPsfVm& virtualMachine, const char* pathString, CPsfStreamProvider* streamProvider, CPsfBase::TagMap* tags)
{
	Iop::PsfSubSystemPtr subSystem = Iop::PsfSubSystemPtr(new Iop::CPsfSubSystem());
	virtualMachine.SetSubSystem(subSystem);

	{
		Iop::CSubSystem::BiosPtr bios = Iop::CSubSystem::BiosPtr(new CPsxBios(virtualMachine.GetCpu(), virtualMachine.GetRam(), PS2::IOP_RAM_SIZE));
		subSystem->SetBios(bios);
		LoadPsxRecurse(virtualMachine, static_cast<CPsxBios*>(bios.get()), pathString, streamProvider, tags);
	}
}

void CPsfLoader::LoadPsxRecurse(CPsfVm& virtualMachine, CPsxBios* bios, const char* pathString, CPsfStreamProvider* streamProvider, CPsfBase::TagMap* tags)
{
	Framework::CStream* input(streamProvider->GetStreamForPath(pathString));
	CPsfBase psfFile(*input);
	delete input;

	if(psfFile.GetVersion() != CPsfBase::VERSION_PLAYSTATION)
	{
		throw std::runtime_error("Not a PlayStation psf.");
	}
	const char* libPath = psfFile.GetTagValue("_lib");
	if(tags)
	{
		tags->insert(psfFile.GetTagsBegin(), psfFile.GetTagsEnd());
	}
	{
		uint32 sp = 0, pc = 0;
		if(libPath != NULL)
		{
			filesystem::path path(pathString); 
			path = path.branch_path() / libPath;
			LoadPsxRecurse(virtualMachine, bios, path.generic_string().c_str(), streamProvider);
			sp = virtualMachine.GetCpu().m_State.nGPR[CMIPS::SP].nV0;
			pc = virtualMachine.GetCpu().m_State.nPC;
		}
        bios->LoadExe(psfFile.GetProgram());
		if(sp != 0)
		{
			virtualMachine.GetCpu().m_State.nGPR[CMIPS::SP].nD0 = static_cast<int32>(sp);
		}
		if(pc != 0)
		{
			virtualMachine.GetCpu().m_State.nPC = pc;
		}
	}
	{
		unsigned int currentLib = 2;
		while(1)
		{
			std::string libName = "_lib" + boost::lexical_cast<std::string>(currentLib);
			const char* libPath = psfFile.GetTagValue(libName.c_str());
			if(libPath == NULL)
			{
				break;
			}
			filesystem::path path(pathString); 
			path = path.branch_path() / libPath;
			uint32 sp = virtualMachine.GetCpu().m_State.nGPR[CMIPS::SP].nV0;
			uint32 pc = virtualMachine.GetCpu().m_State.nPC;
			LoadPsxRecurse(virtualMachine, bios, path.generic_string().c_str(), streamProvider);
			virtualMachine.GetCpu().m_State.nGPR[CMIPS::SP].nD0 = static_cast<int32>(sp);
			virtualMachine.GetCpu().m_State.nPC = pc;
			currentLib++;
		}
	}
}

void CPsfLoader::LoadPs2(CPsfVm& virtualMachine, const char* pathString, CPsfStreamProvider* streamProvider, CPsfBase::TagMap* tags)
{
	Iop::PsfSubSystemPtr subSystem = Iop::PsfSubSystemPtr(new Iop::CPsfSubSystem());
	virtualMachine.SetSubSystem(subSystem);

	{
		Iop::CSubSystem::BiosPtr bios = Iop::CSubSystem::BiosPtr(new PS2::CPsfBios(virtualMachine.GetCpu(), virtualMachine.GetRam(), PS2::IOP_RAM_SIZE));
		subSystem->SetBios(bios);
	    LoadPs2Recurse(virtualMachine, static_cast<PS2::CPsfBios*>(bios.get()), pathString, streamProvider, tags);
		static_cast<PS2::CPsfBios*>(bios.get())->Start();
	}
}

void CPsfLoader::LoadPs2Recurse(CPsfVm& virtualMachine, PS2::CPsfBios* bios, const char* pathString, CPsfStreamProvider* streamProvider, CPsfBase::TagMap* tags)
{
	Framework::CStream* input(streamProvider->GetStreamForPath(pathString));
	CPsfBase psfFile(*input);
	delete input;

	if(psfFile.GetVersion() != CPsfBase::VERSION_PLAYSTATION2)
	{
		throw std::runtime_error("Not a PlayStation2 psf.");
	}
	const char* libPath = psfFile.GetTagValue("_lib");
	if(tags != NULL)
	{
		tags->insert(psfFile.GetTagsBegin(), psfFile.GetTagsEnd());
	}
	if(libPath != NULL)
	{
		filesystem::path path(pathString); 
		path = path.branch_path() / libPath;
		LoadPs2Recurse(virtualMachine, bios, path.generic_string().c_str(), streamProvider);
	}
	bios->AppendArchive(psfFile);
}

void CPsfLoader::LoadPsp(CPsfVm& virtualMachine, const char* pathString, CPsfStreamProvider* streamProvider, CPsfBase::TagMap* tags)
{
	Psp::PsfSubSystemPtr subSystem = Psp::PsfSubSystemPtr(new Psp::CPsfSubSystem(0x00800000));
	virtualMachine.SetSubSystem(subSystem);

	{
		Psp::CPsfBios* bios = &subSystem->GetBios();
		LoadPspRecurse(virtualMachine, bios, pathString, streamProvider, tags);
		bios->Start();
	}
}

void CPsfLoader::LoadPspRecurse(CPsfVm& virtualMachine, Psp::CPsfBios* bios, const char* pathString, CPsfStreamProvider* streamProvider, CPsfBase::TagMap* tags)
{
	Framework::CStream* input(streamProvider->GetStreamForPath(pathString));
	CPsfBase psfFile(*input);
	delete input;

	if(psfFile.GetVersion() != CPsfBase::VERSION_PLAYSTATIONPORTABLE)
	{
		throw std::runtime_error("Not a PlayStation Portable psf.");
	}
	const char* libPath = psfFile.GetTagValue("_lib");
	if(tags != NULL)
	{
		tags->insert(psfFile.GetTagsBegin(), psfFile.GetTagsEnd());
	}
	if(libPath != NULL)
	{
		filesystem::path path(pathString); 
		path = path.branch_path() / libPath;
		LoadPspRecurse(virtualMachine, bios, path.generic_string().c_str(), streamProvider);
	}
	bios->AppendArchive(psfFile);
}
