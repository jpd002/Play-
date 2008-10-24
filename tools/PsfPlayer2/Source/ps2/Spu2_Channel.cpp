#include "Spu2_Channel.h"

using namespace PS2;
using namespace PS2::Spu2;

CChannel::CChannel()
{
	volLeft = 0;
	volRight = 0;
	pitch = 0;
	adsr1 = 0;
	adsr2 = 0;
	envx = 0;
	volxLeft = 0;
	volxRight = 0;
}

CChannel::~CChannel()
{

}
