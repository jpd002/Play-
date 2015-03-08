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
		int				AllocHardTimer(CMIPS&, uint32, uint32, uint32);
		int				ReferHardTimer(uint32, uint32, uint32, uint32);
		void			SetTimerMode(CMIPS&, uint32, uint32);
		int				GetTimerStatus(CMIPS&, uint32);
		void			SetTimerCompare(CMIPS&, uint32, uint32);
		int				GetHardTimerIntrCode(uint32);
		int				SetTimerCallback(CMIPS&, int, uint32, uint32, uint32);
		int				SetupHardTimer(uint32, uint32, uint32, uint32);
		int				StartHardTimer(uint32);

		CIopBios&		m_bios;
	};
}

#endif
