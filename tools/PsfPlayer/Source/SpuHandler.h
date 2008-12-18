#ifndef _SPUHANDLER_H_
#define _SPUHANDLER_H_

#include "iop/Iop_SpuBase.h"

class CSpuHandler
{
public:
	virtual				~CSpuHandler();

	virtual void		Reset() = 0;
	virtual void		Update(Iop::CSpuBase&, Iop::CSpuBase&) = 0;
	virtual bool		HasFreeBuffers() = 0;

protected:
	enum
	{
		SAMPLES_PER_UPDATE = 352,
	};

	void				MixInputs(int16*, unsigned int, unsigned int, Iop::CSpuBase&, Iop::CSpuBase&);

private:

};

#endif
