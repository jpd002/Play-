#ifndef _PS2_SPU2_CHANNEL_H_
#define _PS2_SPU2_CHANNEL_H_

#include "Types.h"

namespace PS2
{
	namespace Spu2
	{
		class CChannel
		{
		public:
							CChannel();
			virtual			~CChannel();

			uint16			volLeft;
			uint16			volRight;
			uint16			pitch;
			uint16			adsr1;
			uint16			adsr2;
			uint16			envx;
			uint16			volxLeft;
			uint16			volxRight;
		};
	}
}

#endif
