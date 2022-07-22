#pragma once

#include <QMdiArea>
#include "ELFView.h"
#include "filesystem_def.h"

template <typename ElfType>
class CElfViewEx : public CELFView<ElfType>
{
public:
	CElfViewEx(QMdiArea*, const fs::path&);
	virtual ~CElfViewEx() = default;

private:
	std::unique_ptr<ElfType> m_elf;
};
