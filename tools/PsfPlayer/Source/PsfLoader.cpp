#include "PsfLoader.h"
#include "StdStream.h"
#include "PsxBios.h"
#include "PsfBios.h"
#include "Ps2Const.h"
#include <boost/filesystem.hpp>
#include <boost/lexical_cast.hpp>

using namespace Framework;
using namespace std;
namespace filesystem = boost::filesystem;

void CPsfLoader::LoadPsf(CPsfVm& virtualMachine, const char* pathString, CPsfBase::TagMap* tags)
{
	size_t pathLength = strlen(pathString);
	if(pathString[pathLength - 1] == '2')
	{
		LoadPs2(virtualMachine, pathString, tags);
	}
	else
	{
		LoadPsx(virtualMachine, pathString, tags);
	}
}

void CPsfLoader::LoadPsx(CPsfVm& virtualMachine, const char* pathString, CPsfBase::TagMap* tags)
{
    CPsxBios* bios = new CPsxBios(virtualMachine.GetCpu(), virtualMachine.GetRam(), PS2::IOP_RAM_SIZE);
    virtualMachine.SetBios(bios);
    LoadPsxRecurse(virtualMachine, bios, pathString, tags);
}

void CPsfLoader::LoadPsxRecurse(CPsfVm& virtualMachine, CPsxBios* bios, const char* pathString, CPsfBase::TagMap* tags)
{
	CStdStream input(pathString, "rb");
	CPsfBase psfFile(input);
	if(psfFile.GetVersion() != CPsfBase::VERSION_PLAYSTATION)
	{
		throw runtime_error("Not a PlayStation psf.");
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
			LoadPsxRecurse(virtualMachine, bios, path.string().c_str());
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
			string libName = "_lib" + boost::lexical_cast<string>(currentLib);
			const char* libPath = psfFile.GetTagValue(libName.c_str());
			if(libPath == NULL)
			{
				break;
			}
			filesystem::path path(pathString); 
			path = path.branch_path() / libPath;
			uint32 sp = virtualMachine.GetCpu().m_State.nGPR[CMIPS::SP].nV0;
			uint32 pc = virtualMachine.GetCpu().m_State.nPC;
			LoadPsxRecurse(virtualMachine, bios, path.string().c_str());
			virtualMachine.GetCpu().m_State.nGPR[CMIPS::SP].nD0 = static_cast<int32>(sp);
			virtualMachine.GetCpu().m_State.nPC = pc;
			currentLib++;
		}
	}
}

void CPsfLoader::LoadPs2(CPsfVm& virtualMachine, const char* pathString, CPsfBase::TagMap* tags)
{
	PS2::CPsfBios* bios = new PS2::CPsfBios(virtualMachine.GetCpu(), virtualMachine.GetRam(), PS2::IOP_RAM_SIZE);
    virtualMachine.SetBios(bios);
	LoadPs2Recurse(virtualMachine, bios, pathString, tags);
	bios->Start();
}

void CPsfLoader::LoadPs2Recurse(CPsfVm& virtualMachine, PS2::CPsfBios* bios, const char* pathString, CPsfBase::TagMap* tags)
{
	CStdStream input(pathString, "rb");
	CPsfBase psfFile(input);
	if(psfFile.GetVersion() != CPsfBase::VERSION_PLAYSTATION2)
	{
		throw runtime_error("Not a PlayStation2 psf.");
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
		LoadPs2Recurse(virtualMachine, bios, path.string().c_str());
	}
	bios->AppendArchive(psfFile);
}
