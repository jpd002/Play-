#ifndef _JITTER_CODEGEN_H_
#define _JITTER_CODEGEN_H_

#include "Stream.h"
#include "Jitter_Statement.h"

namespace Jitter
{
	class CCodeGen
	{
	public:
		virtual					~CCodeGen() {};

		virtual void			SetStream(Framework::CStream*) = 0;
		virtual void			GenerateCode(const StatementList&, unsigned int) = 0;
		virtual unsigned int	GetAvailableRegisterCount() const = 0;
	};
}

#endif
