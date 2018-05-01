#pragma once

namespace Iop
{
	class CSifMan;

	class CSifModuleProvider
	{
	public:
		virtual void RegisterSifModules(CSifMan&) = 0;
	};
}
