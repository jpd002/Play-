#pragma once

#include "input/InputProvider.h"
#include <emscripten/html5.h>

class CInputProviderEmscripten : public CInputProvider
{
public:
	uint32 GetId() const override;
	std::string GetTargetDescription(const BINDINGTARGET&) const override;

	static BINDINGTARGET MakeBindingTarget(const EM_UTF8* code);

	void OnKeyDown(const EM_UTF8*);
	void OnKeyUp(const EM_UTF8*);
};
