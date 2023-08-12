#include "FunctionsView.h"
#include <QPushButton>
#include "ui_TagsView.h"
#include "ELF.h"

CFunctionsView::CFunctionsView(QMdiArea* parent)
    : CTagsView(parent)
{
	parent->addSubWindow(this);
	setWindowTitle("Functions");

	auto btnImport = new QPushButton("Load ELF symbols", this);
	ui->bottomButtonsLayout->addWidget(btnImport);

	connect(btnImport, &QPushButton::clicked, this, &CFunctionsView::OnImportClick);
}

void CFunctionsView::SetContext(CMIPS* context, CBiosDebugInfoProvider* biosDebugInfoProvider)
{
	CTagsView::SetContext(context, &context->m_Functions, biosDebugInfoProvider);
}

void CFunctionsView::OnImportClick()
{
	if(!m_context) return;

	for(auto moduleIterator(std::begin(m_modules));
	    std::end(m_modules) != moduleIterator; moduleIterator++)
	{
		const auto& module(*moduleIterator);
		auto moduleImage = reinterpret_cast<CELF32*>(module.param);
		if(!moduleImage) continue;

		moduleImage->EnumerateSymbols([&](const ELF::ELFSYMBOL32& symbol, uint8 type, uint8 binding, const char* name) {
			if(type == ELF::STT_FUNC)
			{
				//NOTE: This check for the section owning the symbol might not be necessary.
				//Since we load the executable at the address specified by the program header
				auto symbolSection = moduleImage->GetSection(symbol.nSectionIndex);
				if(!symbolSection) return;
				m_context->m_Functions.InsertTag(module.begin + (symbol.nValue - symbolSection->nStart), name);
			}
		});
	}

	RefreshList();
	OnStateChange();
}
