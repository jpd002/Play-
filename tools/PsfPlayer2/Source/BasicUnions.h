#ifndef _BASICUNIONS_H_
#define _BASICUNIONS_H_

struct UNION32_16
{
	union
	{
		struct
		{
			uint16 h0;
			uint16 h1;
		};
		uint32 w;
	};
};

#endif
