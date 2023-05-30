#pragma once

#include "PspModule.h"

namespace Psp
{
	class CUmdUser : public CModule
	{
	public:
		std::string GetName() const override;
		void Invoke(uint32, CMIPS&) override;

	private:
		int32 UmdWaitDriveStateCB(uint32);
	};
}
