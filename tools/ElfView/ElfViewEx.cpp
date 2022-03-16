#include "ElfViewEx.h"
#include "StdStreamUtils.h"
#include "ElfFile.h"

CElfViewEx::CElfViewEx(QMdiArea* mdiArea, const fs::path& elfPath)
    : CELFView(mdiArea)
{
	setWindowTitle(QString::fromStdString(elfPath.filename().string()));
	auto stream = Framework::CreateInputStdStream(elfPath.native());
	m_elf = new CElfFile(stream);
	SetELF(m_elf);
}

CElfViewEx::~CElfViewEx()
{
	delete m_elf;
}
