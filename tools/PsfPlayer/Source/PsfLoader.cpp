#include "PsfLoader.h"
#include "StdStream.h"

#include "psx/PsxBios.h"
#include "iop/IopBios.h"
#include "ps2/Ps2_PsfDevice.h"
#include "Iop_PsfSubSystem.h"
#include "Ps2Const.h"

#include "psp/Psp_PsfSubSystem.h"

#include <boost/filesystem.hpp>

namespace filesystem = boost::filesystem;

void CPsfLoader::LoadPsf(CPsfVm& virtualMachine, const CPsfPathToken& filePath, const filesystem::path& archivePath, CPsfBase::TagMap* tags)
{
	auto streamProvider = CreatePsfStreamProvider(archivePath);

	auto pathString = filePath.GetWidePath();
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

void CPsfLoader::LoadPsx(CPsfVm& virtualMachine, const CPsfPathToken& filePath, CPsfStreamProvider* streamProvider, CPsfBase::TagMap* tags)
{
	auto subSystem = std::make_shared<Iop::CPsfSubSystem>(false);
	virtualMachine.SetSubSystem(subSystem);

	auto bios = dynamic_cast<CPsxBios*>(subSystem->GetBios());
	LoadPsxRecurse(bios, subSystem->GetCpu(), filePath, streamProvider, tags);
}

void CPsfLoader::LoadPsxRecurse(CPsxBios* bios, CMIPS& cpu, const CPsfPathToken& filePath, CPsfStreamProvider* streamProvider, CPsfBase::TagMap* tags)
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
		if(libPath != nullptr)
		{
			auto libFilePath = streamProvider->GetSiblingPath(filePath, libPath);
			LoadPsxRecurse(bios, cpu, libFilePath, streamProvider);
			sp = cpu.m_State.nGPR[CMIPS::SP].nV0;
			pc = cpu.m_State.nPC;
		}

		//Patch the text section size in the header because some PSF sets got it wrong.
		CPsxBios::EXEHEADER* fileHeader = reinterpret_cast<CPsxBios::EXEHEADER*>(psfFile.GetProgram());
		fileHeader->textSize = psfFile.GetProgramUncompressedSize() - 0x800;

		bios->LoadExe(reinterpret_cast<uint8*>(fileHeader));
		if(sp != 0)
		{
			cpu.m_State.nGPR[CMIPS::SP].nD0 = static_cast<int32>(sp);
		}
		if(pc != 0)
		{
			cpu.m_State.nPC = pc;
		}
	}
	{
		unsigned int currentLib = 2;
		while(1)
		{
			std::string libName = "_lib" + std::to_string(currentLib);
			const char* libPath = psfFile.GetTagValue(libName.c_str());
			if(libPath == nullptr)
			{
				break;
			}
			auto libFilePath = streamProvider->GetSiblingPath(filePath, libPath);
			uint32 sp = cpu.m_State.nGPR[CMIPS::SP].nV0;
			uint32 pc = cpu.m_State.nPC;
			LoadPsxRecurse(bios, cpu, libFilePath, streamProvider);
			cpu.m_State.nGPR[CMIPS::SP].nD0 = static_cast<int32>(sp);
			cpu.m_State.nPC = pc;
			currentLib++;
		}
	}
}

void CPsfLoader::LoadPs2(CPsfVm& virtualMachine, const CPsfPathToken& filePath, CPsfStreamProvider* streamProvider, CPsfBase::TagMap* tags)
{
	static const char* g_psfDeviceName = "psf";

	auto subSystem = std::make_shared<Iop::CPsfSubSystem>(true);
	virtualMachine.SetSubSystem(subSystem);

	auto bios = dynamic_cast<CIopBios*>(subSystem->GetBios());
	auto psfDevice = std::make_shared<PS2::CPsfDevice>();

	//Setup IOMAN hooks
	{
		auto ioman = bios->GetIoman();
		ioman->RegisterDevice(g_psfDeviceName, psfDevice);
		ioman->RegisterDevice("host0", psfDevice);
		ioman->RegisterDevice("hefile", psfDevice);
	}

	LoadPs2Recurse(psfDevice.get(), filePath, streamProvider, tags);

	{
		auto execPath = std::string(g_psfDeviceName) + ":/psf2.irx";
		auto moduleId = bios->LoadModule(execPath.c_str());
		assert(moduleId >= 0);
		bios->StartModule(moduleId, execPath.c_str(), nullptr, 0);
	}
}

void CPsfLoader::LoadPs2Recurse(PS2::CPsfDevice* psfDevice, const CPsfPathToken& filePath, CPsfStreamProvider* streamProvider, CPsfBase::TagMap* tags)
{
	Framework::CStream* input(streamProvider->GetStreamForPath(filePath));
	CPsfBase psfFile(*input);
	delete input;

	if(psfFile.GetVersion() != CPsfBase::VERSION_PLAYSTATION2)
	{
		throw std::runtime_error("Not a PlayStation2 psf.");
	}
	const char* libPath = psfFile.GetTagValue("_lib");
	if(tags != nullptr)
	{
		tags->insert(psfFile.GetTagsBegin(), psfFile.GetTagsEnd());
	}
	if(libPath != nullptr)
	{
		auto libFilePath = streamProvider->GetSiblingPath(filePath, libPath);
		LoadPs2Recurse(psfDevice, libFilePath, streamProvider);
	}
	psfDevice->AppendArchive(psfFile);
}

void CPsfLoader::LoadPsp(CPsfVm& virtualMachine, const CPsfPathToken& filePath, CPsfStreamProvider* streamProvider, CPsfBase::TagMap* tags)
{
	auto subSystem = std::make_shared<Psp::CPsfSubSystem>(0x00800000);
	virtualMachine.SetSubSystem(subSystem);

	{
		auto bios = &subSystem->GetBios();
		LoadPspRecurse(bios, filePath, streamProvider, tags);
		bios->Start();
	}
}

void CPsfLoader::LoadPspRecurse(Psp::CPsfBios* bios, const CPsfPathToken& filePath, CPsfStreamProvider* streamProvider, CPsfBase::TagMap* tags)
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
		auto libFilePath = streamProvider->GetSiblingPath(filePath, libPath);
		LoadPspRecurse(bios, libFilePath, streamProvider);
	}
	bios->AppendArchive(psfFile);
}
