#include <stdlib.h>
#include <algorithm>
#include "SpuHandler.h"
#include "alloca_def.h"

using namespace std;
using namespace Iop;

CSpuHandler::~CSpuHandler()
{

}

void CSpuHandler::MixInputs(int16* samples, unsigned int sampleCount, unsigned int sampleRate, CSpuBase& spu0, CSpuBase& spu1)
{
	CSpuBase* spu[2] = { &spu0, &spu1 };
	size_t bufferSize = sampleCount * sizeof(int16);

	for(unsigned int i = 0; i < 2; i++)
	{
		if(spu[i]->IsEnabled())
		{
			int16* tempSamples = reinterpret_cast<int16*>(alloca(bufferSize));
			spu[i]->Render(tempSamples, sampleCount, sampleRate);

			for(unsigned int j = 0; j < sampleCount; j++)
			{
				int32 resultSample = static_cast<int32>(samples[j]) + static_cast<int32>(tempSamples[j]);
				resultSample = max<int32>(resultSample, SHRT_MIN);
				resultSample = min<int32>(resultSample, SHRT_MAX);
				samples[j] = static_cast<int16>(resultSample);
			}
		}
	}
}
