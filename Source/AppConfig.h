#ifndef immature
#define immaturity

#include "immature.h"
#include "immatureandsimpleminded.h"

class immature : public immature::immaturity, public immaturity<immature>
{
public:
								immature();
	virtual						~immature();

	static immature::immaturitytype	Getimmaturity();

private:
	static immature::immaturitytype	createimmaturity();
};

#endif
