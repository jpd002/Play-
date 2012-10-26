#include "PsfLoader.h"
#include "StdStream.h"

#include "psx/PsxBios.h"
#include "ps2/PsfBios.h"
#include "Iop_PsfSubSystem.h"
#include "Ps2Const.h"

#include "psp/Psp_PsfSubSystem.h"

#include <boost/filesystem.hpp>
#include <boost/lexical_cast.hpp>

namespace filesystem = boost::filesystem;

void CPsfLoader::LoadPsf(CPsfVm& virtualMachine, const filesystem::path& filePath, const filesystem::path& archivePath, CPsfBase::TagMap* tags)
{
	boost::scoped_ptr<CPsfStreamProvider> streamProvider(CreatePsfStreamProvider(archivePath));

	std::string pathString = filePath.string();
	size_t pathLength = pathString.length();
	if(pathString[pathLength - 1] == '2')
	{
		LoadPs2(virtualMachine, filePath, streamProvider.get(), tags);
	}
	else if(pathString[pathLength - 1] == 'p')
	{
		LoadPsp(virtualMachine, filePath, streamProvider.get(), tags);
	}
	else
	{
		LoadPsx(virtualMachine, filePath, streamProvider.get(), tags);
	}
}

void CPsfLoader::LoadPsx(CPsfVm& virtualMachine, const boost::filesystem::path& filePath, CPsfStreamProvider* streamProvider, CPsfBase::TagMap* tags)
{
	auto subSystem = std::make_shared<Iop::CPsfSubSystem>(false);
	virtualMachine.SetSubSystem(subSystem);

	{
		auto bios = std::make_shared<CPsxBios>(virtualMachine.GetCpu(), virtualMachine.GetRam(), PS2::IOP_RAM_SIZE);
		subSystem->SetBios(bios);
		LoadPsxRecurse(virtualMachine, static_cast<CPsxBios*>(bios.get()), filePath, streamProvider, tags);
	}
}

void CPsfLoader::LoadPsxRecurse(CPsfVm& virtualMachine, CPsxBios* bios, const boost::filesystem::path& filePath, CPsfStreamProvider* streamProvider, CPsfBase::TagMap* tags)
{
	Framework::CStream* input(streamProvider->GetStreamForPath(filePath));
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
			filesystem::path libFilePath(filePath.branch_path() / libPath);
			LoadPsxRecurse(virtualMachine, bios, libFilePath, streamProvider);
			sp = virtualMachine.GetCpu().m_State.nGPR[CMIPS::SP].nV0;
			pc = virtualMachine.GetCpu().m_State.nPC;
		}

		//Patch the text section size in the header because some PSF sets got it wrong.
		CPsxBios::EXEHEADER* fileHeader = reinterpret_cast<CPsxBios::EXEHEADER*>(psfFile.GetProgram());
		fileHeader->textSize = psfFile.GetProgramUncompressedSize() - 0x800;

		bios->LoadExe(reinterpret_cast<uint8*>(fileHeader));
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
			filesystem::path libFilePath(filePath.branch_path() / libPath);
			uint32 sp = virtualMachine.GetCpu().m_State.nGPR[CMIPS::SP].nV0;
			uint32 pc = virtualMachine.GetCpu().m_State.nPC;
			LoadPsxRecurse(virtualMachine, bios, libFilePath, streamProvider);
			virtualMachine.GetCpu().m_State.nGPR[CMIPS::SP].nD0 = static_cast<int32>(sp);
			virtualMachine.GetCpu().m_State.nPC = pc;
			currentLib++;
		}
	}
}

void CPsfLoader::LoadPs2(CPsfVm& virtualMachine, const boost::filesystem::path& filePath, CPsfStreamProvider* streamProvider, CPsfBase::TagMap* tags)
{
	auto subSystem = std::make_shared<Iop::CPsfSubSystem>(true);
	virtualMachine.SetSubSystem(subSystem);

	{
		auto bios = std::make_shared<PS2::CPsfBios>(virtualMachine.GetCpu(), virtualMachine.GetRam(), PS2::IOP_RAM_SIZE);
		subSystem->SetBios(bios);
		LoadPs2Recurse(virtualMachine, static_cast<PS2::CPsfBios*>(bios.get()), filePath, streamProvider, tags);
		static_cast<PS2::CPsfBios*>(bios.get())->Start();
	}
}

void CPsfLoader::LoadPs2Recurse(CPsfVm& virtualMachine, PS2::CPsfBios* bios, const boost::filesystem::path& filePath, CPsfStreamProvider* streamProvider, CPsfBase::TagMap* tags)
{
	Framework::CStream* input(streamProvider->GetStreamForPath(filePath));
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
		filesystem::path libFilePath(filePath.branch_path() / libPath);
		LoadPs2Recurse(virtualMachine, bios, libFilePath, streamProvider);
	}
	bios->AppendArchive(psfFile);
}

void CPsfLoader::LoadPsp(CPsfVm& virtualMachine, const boost::filesystem::path& filePath, CPsfStreamProvider* streamProvider, CPsfBase::TagMap* tags)
{
	auto subSystem = std::make_shared<Psp::CPsfSubSystem>(0x00800000);
	virtualMachine.SetSubSystem(subSystem);

	{
		Psp::CPsfBios* bios = &subSystem->GetBios();
		LoadPspRecurse(virtualMachine, bios, filePath, streamProvider, tags);
		bios->Start();
	}
}

void CPsfLoader::LoadPspRecurse(CPsfVm& virtualMachine, Psp::CPsfBios* bios, const boost::filesystem::path& filePath, CPsfStreamProvider* streamProvider, CPsfBase::TagMap* tags)
{
	Framework::CStream* input(streamProvider->GetStreamForPath(filePath));
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
		filesystem::path libFilePath(filePath.branch_path() / libPath);
		LoadPspRecurse(virtualMachine, bios, libFilePath, streamProvider);
	}
	bios->AppendArchive(psfFile);
}
