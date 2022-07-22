#include "ElfViewEx.h"
#include "StdStreamUtils.h"
#include "ElfFile.h"

template <typename ElfType>
CElfViewEx<ElfType>::CElfViewEx(QMdiArea* mdiArea, const fs::path& elfPath)
    : CELFView(mdiArea)
{
	setWindowTitle(QString::fromStdString(elfPath.filename().string()));
	auto stream = Framework::CreateInputStdStream(elfPath.native());
	m_elf = std::make_unique<CElfFile<ElfType>>(stream);
	SetELF(m_elf.get());
}

template class CElfViewEx<CELF32>;
template class CElfViewEx<CELF64>;
