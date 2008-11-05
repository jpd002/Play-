#ifndef _IOP_SPUBASE_H_
#define _IOP_SPUBASE_H_

#include <boost/static_assert.hpp>
#include "Types.h"
#include "convertible.h"

namespace Iop
{
	class CSpuBase
	{
	public:
		struct ADSR_LEVEL : public convertible<uint16>
		{
			unsigned int	sustainLevel	: 4;
			unsigned int	decayRate		: 4;
			unsigned int	attackRate		: 7;
			unsigned int	attackMode		: 1;
		};
		BOOST_STATIC_ASSERT(sizeof(ADSR_LEVEL) >= sizeof(uint16));

		struct ADSR_RATE : public convertible<uint16>
		{
			unsigned int	releaseRate			: 5;
			unsigned int	releaseMode			: 1;
			unsigned int	sustainRate			: 7;
			unsigned int	reserved0			: 1;
			unsigned int	sustainDirection	: 1;
			unsigned int	sustainMode			: 1;
		};
		BOOST_STATIC_ASSERT(sizeof(ADSR_RATE) >= sizeof(uint16));

		struct CHANNEL_VOLUME : public convertible<uint16>
		{
			union
			{
				struct
				{
					unsigned int	unused0		: 15;
					unsigned int	mode		: 1;
				} mode;
				struct
				{
					unsigned int	volume		: 14;
					unsigned int	phase		: 1;
					unsigned int	mode		: 1;
				} volume;
				struct
				{
					unsigned int	volume		: 7;
					unsigned int	unused0		: 5;
					unsigned int	phase		: 1;
					unsigned int	decrease	: 1;
					unsigned int	slope		: 1;
					unsigned int	mode		: 1;
				} sweep;
			};
		};
		BOOST_STATIC_ASSERT(sizeof(CHANNEL_VOLUME) >= sizeof(uint16));

		struct CHANNEL
		{
			CHANNEL_VOLUME	volumeLeft;
			CHANNEL_VOLUME	volumeRight;
			uint16			pitch;
			uint32			address;
			ADSR_LEVEL		adsrLevel;
			ADSR_RATE		adsrRate;
			uint32			adsrVolume;
			uint32			repeat;
			uint16			status;
			uint32          current;
		};

	private:

	};
}

#endif
