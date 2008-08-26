#include "PsfLoader.h"
#include "StdStream.h"
#include <boost/filesystem.hpp>

using namespace Framework;
using namespace Psx;
using namespace boost;
using namespace std;

void CPsfLoader::LoadPsf(CPsxVm& virtualMachine, const char* pathString, CPsfBase::TagMap* tags)
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
	uint32 sp = 0, pc = 0;
	if(libPath != NULL)
	{
		filesystem::path path(pathString); 
		path = path.branch_path() / libPath;
		LoadPsf(virtualMachine, path.string().c_str());
		sp = virtualMachine.GetCpu().m_State.nGPR[CMIPS::SP].nV0;
		pc = virtualMachine.GetCpu().m_State.nPC;
	}
	virtualMachine.LoadExe(psfFile.GetProgram());
	if(sp != 0)
	{
		virtualMachine.GetCpu().m_State.nGPR[CMIPS::SP].nD0 = static_cast<int32>(sp);
	}
	if(pc != 0)
	{
		virtualMachine.GetCpu().m_State.nPC = pc;
	}
}
