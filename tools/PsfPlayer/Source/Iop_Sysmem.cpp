#include "Iop_Sysmem.h"

using namespace Iop;
using namespace std;

CSysmem::CSysmem()
{

}

CSysmem::~CSysmem()
{

}

string CSysmem::GetId() const
{
    return "sysmem";
}

void CSysmem::Invoke(CMIPS& context, unsigned int commandId)
{

}
