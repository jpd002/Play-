#ifndef _TIMRMAN_H_
#define _TIMRMAN_H_

#include "Iop_Module.h"

class CIopBios;

namespace Iop
{
	class CTimrman : public CModule
	{
	public:
						CTimrman(CIopBios&);
		virtual			~CTimrman();

		std::string		GetId() const;
		std::string		GetFunctionName(unsigned int) const;
		void			Invoke(CMIPS&, unsigned int);

	private:
		int				AllocHardTimer(int, int, int);
		int				SetTimerCallback(CMIPS&, int, uint32, uint32, uint32);
		int				UnknownTimerFunction22(uint32, uint32, uint32, uint32);
		int				UnknownTimerFunction23(uint32);

		CIopBios&		m_bios;
	};
}

#endif
