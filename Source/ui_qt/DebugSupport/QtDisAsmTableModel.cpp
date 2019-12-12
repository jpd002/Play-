#include <QPainter>

#include "QtDisAsmTableModel.h"
#include "string_cast.h"
#include "string_format.h"
#include "lexical_cast_ex.h"

CQtDisAsmTableModel::CQtDisAsmTableModel(QObject* parent, CVirtualMachine& virtualMachine, CMIPS* context)
    : QAbstractTableModel(parent)
	, m_virtualMachine(virtualMachine)
    , m_ctx(context)
    , m_instructionSize(4)
    , m_disAsmType(DISASM_TYPE::DISASM_STANDARD)
{
	std::vector<std::string> headers = {"S", "Address", "R", "Instr","I-Mn", "I-Op", "Target"};
	m_headers.clear();
	for(int i = 0; i < headers.size(); ++i)
		m_headers.insert(i, QVariant(headers[i].c_str()));

	QPen pen;
	pen.setWidth(3.3);
	pen.setColor(Qt::white);
	pen.setCapStyle(Qt::RoundCap);
	pen.setJoinStyle(Qt::MiterJoin);
	{
		start_line.fill(Qt::transparent);

		QPainterPath start_path;
		start_path.moveTo(12.5, 25);
		start_path.lineTo(12.5, 12.5);
		start_path.lineTo(25, 12.5);

		QPainter painter(&start_line);
		painter.setPen(pen);
		painter.drawPath(start_path);
	}
	{
		end_line.fill(Qt::transparent);

		QPainterPath end_path;
		end_path.moveTo(12.5, 0);
		end_path.lineTo(12.5, 12.5);
		end_path.lineTo(25, 12.5);

		QPainter painter(&end_line);
		painter.setPen(pen);
		painter.drawPath(end_path);
	}
	{
		line.fill(Qt::transparent);

		QPainterPath line_path;
		line_path.moveTo(12.5, 0);
		line_path.lineTo(12.5, 25);

		QPainter painter(&line);
		painter.setPen(pen);
		painter.drawPath(line_path);
	}
	{
		arrow.fill(Qt::transparent);

		QPainterPath line_path;
		line_path.moveTo(12.5, 0);
		line_path.lineTo(25, 12.5);
		line_path.lineTo(12.5, 25);

		line_path.moveTo(25, 12.5);
		line_path.lineTo(0, 12.5);


		QPainter painter(&arrow);
		painter.setPen(pen);
		painter.drawPath(line_path);
	}
	{
		breakpoint.fill(Qt::transparent);

		QRadialGradient gradient(12, 12, 12, 12, 12);
		gradient.setColorAt(0, QColor::fromRgbF(1, 0, 0, 1));
		gradient.setColorAt(1, QColor::fromRgbF(1, 0.7, 0.7, 1));
		QBrush brush(gradient);

		pen.setWidth(1);
		QPainter painter(&breakpoint);
		painter.setBrush(brush);
		painter.setPen(pen);
		painter.drawEllipse(12, 12, 12, 12);
	}
		{
		breakpoint_arrow.fill(Qt::transparent);

		QRadialGradient gradient(12, 12, 12, 12, 12);
		gradient.setColorAt(0, QColor::fromRgbF(1, 0, 0, 1));
		gradient.setColorAt(1, QColor::fromRgbF(1, 0.7, 0.7, 1));
		QBrush brush(gradient);

		QPainterPath path;
		path.moveTo(12.5, 0);
		path.lineTo(25, 12.5);
		path.lineTo(12.5, 25);

		path.moveTo(25, 12.5);
		path.lineTo(0, 12.5);

		QPainter painter(&breakpoint_arrow);
		painter.setBrush(brush);
		painter.setPen(pen);
		painter.drawEllipse(12, 12, 12, 12);

		painter.setBrush(Qt::NoBrush);
		pen.setWidth(3);
		painter.setPen(pen);
		painter.drawPath(path);

	}
}

CQtDisAsmTableModel::~CQtDisAsmTableModel()
{
}

int CQtDisAsmTableModel::rowCount(const QModelIndex& /*parent*/) const
{
	return 0xFFFFFF;
}

int CQtDisAsmTableModel::columnCount(const QModelIndex& /*parent*/) const
{
	return m_headers.size();
}

QVariant CQtDisAsmTableModel::data(const QModelIndex& index, int role) const
{
	// "S", "Address", "R", "I-Mn", "I-Op", "Name",
	uint32 address = (index.row() * m_instructionSize);
	if(role == Qt::DisplayRole)
	{
		switch(index.column())
		{
			case 0://SYMBOL:
				return QVariant();
			break;
			case 1://Address:
				return string_format(("%08X"), address).c_str();
			break;
			case 2://Relation:
				return QVariant();
			break;
		}
		auto size = (m_disAsmType == DISASM_TYPE::DISASM_STANDARD) ? 3 : 5;
		auto subindex = index.column() - 3;
		if(subindex < size)
			return GetInstructionDetails(subindex, address).c_str();
		else if(subindex == size)
			return GetInstructionMetadata(address).c_str();
	}
	if(role == Qt::SizeHintRole)
	{
		if(index.column() == 0 || index.column() == 2)
		{
			return line.size();
		}
	}
	if(role == Qt::DecorationRole)
	{
		switch(index.column())
		{
			case 0:
			{
				bool isBreakpoint = (m_ctx->m_breakpoints.find(address) != std::end(m_ctx->m_breakpoints));
				bool isPC = (m_virtualMachine.GetStatus() != CVirtualMachine::RUNNING && address == m_ctx->m_State.nPC);

				//Draw current instruction arrow and breakpoint icon
				if(isBreakpoint && isPC)
				{
					return breakpoint_arrow;
				}
				//Draw breakpoint icon
				if(isBreakpoint)
				{
					return breakpoint;
				}
				//Draw current instruction arrow
				if(isPC)
				{
					return arrow;
				}
			}
			break;
			case 2:
			{
				const auto* sub = m_ctx->m_analysis->FindSubroutine(address);
				if(sub != nullptr)
				{
					if(address == sub->start)
					{
						return start_line;
					}
					else if(address == sub->end)
					{
						return end_line;
					}
					else
					{
						return line;
					}
				}
			}
		}
	}
	return QVariant();
}

QVariant CQtDisAsmTableModel::headerData(int section, Qt::Orientation orientation, int role) const
{
	if(orientation == Qt::Horizontal)
	{
		if(role == Qt::DisplayRole)
		{
			if(section < m_headers.size())
			{
				return m_headers.at(section);
			}
		}
	}

	return QAbstractTableModel::headerData(section, orientation, role);
}

void CQtDisAsmTableModel::DoubleClicked(const QModelIndex& index, QWidget* parent)
{
	// TODO signal
}

void CQtDisAsmTableModel::Redraw()
{
	emit QAbstractTableModel::dataChanged(index(0, 0), index(rowCount(), columnCount()));
}

void CQtDisAsmTableModel::Redraw(uint32 address)
{
	emit QAbstractTableModel::dataChanged(index(address / m_instructionSize, 0), index(address / m_instructionSize, columnCount()));
}

uint32 CQtDisAsmTableModel::GetInstruction(uint32 address) const
{
	//Address translation perhaps?
	return m_ctx->m_pMemoryMap->GetInstruction(address);
}

std::string CQtDisAsmTableModel::GetInstructionDetails(int index, uint32 address) const
{
	assert((address & 0x07) == 0);

	uint32 instruction = GetInstruction(address);
	switch(index)
	{
		case 0:
		{
			std::string instructionCode = lexical_cast_hex<std::string>(instruction, 8);
			return instructionCode;
		}
		case 1:
		{
			char disAsm[256];
			m_ctx->m_pArch->GetInstructionMnemonic(m_ctx, address + 4, instruction, disAsm, 256);
			return disAsm;
		}
		case 2:
		{
			char disAsm[256];
			m_ctx->m_pArch->GetInstructionOperands(m_ctx, address + 4, instruction, disAsm, 256);
			return disAsm;
		}
	}
}

std::string CQtDisAsmTableModel::GetInstructionMetadata(uint32 address) const
{
	bool commentDrawn = false;
	std::string disAsm;
	//Draw function name
	{
		const char* tag = m_ctx->m_Functions.Find(address);
		if(tag != nullptr)
		{
			disAsm += ("@");
			disAsm += tag;
			commentDrawn = true;
		}
	}

	//Draw target function call if applicable
	if(!commentDrawn)
	{
		uint32 opcode = GetInstruction(address);
		if(m_ctx->m_pArch->IsInstructionBranch(m_ctx, address, opcode) == MIPS_BRANCH_NORMAL)
		{
			uint32 effAddr = m_ctx->m_pArch->GetInstructionEffectiveAddress(m_ctx, address, opcode);
			const char* tag = m_ctx->m_Functions.Find(effAddr);
			if(tag != nullptr)
			{
				disAsm += ("-> ");
				disAsm += tag;
				commentDrawn = true;
			}
		}
	}
	return disAsm.c_str();
}
