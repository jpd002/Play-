#include <QGuiApplication>
#include <QPalette>
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
	m_headers = {"S", "Address", "R", "Instr", "I-Mn", "I-Op", "Target"};

	auto size = m_start_line.size();
	auto start = 2;
	auto middle = size.width() / 2;
	auto end = size.width() - 2;

	QPalette palette = QGuiApplication::palette();
	QPen pen;
	pen.setWidth(2);
	pen.setColor(palette.color(QPalette::WindowText));
	pen.setCapStyle(Qt::RoundCap);
	pen.setJoinStyle(Qt::MiterJoin);
	{
		m_start_line.fill(Qt::transparent);

		QPainterPath start_path;
		start_path.moveTo(middle, end);
		start_path.lineTo(middle, middle);
		start_path.lineTo(end, middle);

		QPainter painter(&m_start_line);
		painter.setPen(pen);
		painter.drawPath(start_path);
	}
	{
		m_end_line.fill(Qt::transparent);

		QPainterPath end_path;
		end_path.moveTo(middle, start);
		end_path.lineTo(middle, middle);
		end_path.lineTo(end, middle);

		QPainter painter(&m_end_line);
		painter.setPen(pen);
		painter.drawPath(end_path);
	}
	{
		m_line.fill(Qt::transparent);

		QPainterPath line_path;
		line_path.moveTo(middle, start);
		line_path.lineTo(middle, end);

		QPainter painter(&m_line);
		painter.setPen(pen);
		painter.drawPath(line_path);
	}
	QPainterPath arrow_path;
	{
		m_arrow.fill(Qt::transparent);

		arrow_path.moveTo(middle, start);
		arrow_path.lineTo(end, middle);
		arrow_path.lineTo(middle, end);

		arrow_path.lineTo(middle, middle + 3);

		arrow_path.lineTo(start, middle + 3);
		arrow_path.lineTo(start, middle - 3);

		arrow_path.lineTo(middle, middle - 3);

		arrow_path.closeSubpath();

		QPainter painter(&m_arrow);
		pen.setColor(palette.color(QPalette::WindowText));
		painter.setPen(pen);
		painter.setBrush(QBrush(Qt::yellow));
		painter.drawPath(arrow_path);
	}
	{
		m_breakpoint.fill(Qt::transparent);

		QPainter painter(&m_breakpoint);
		painter.setBrush(QBrush(Qt::darkRed));
		painter.setPen(Qt::NoPen);
		painter.drawEllipse(QPointF(middle, middle), middle - 1, middle - 1);
	}
	{
		m_breakpoint_arrow.fill(Qt::transparent);

		QPainter painter(&m_breakpoint_arrow);
		painter.setBrush(QBrush(Qt::darkRed));
		painter.setPen(Qt::NoPen);
		painter.drawEllipse(QPointF(middle, middle), middle - 1, middle - 1);

		painter.setBrush(QBrush(Qt::yellow));
		pen.setColor(palette.color(QPalette::WindowText));
		painter.setPen(pen);
		painter.drawPath(arrow_path);
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
		case 0: //SYMBOL:
			return QVariant();
			break;
		case 1: //Address:
			return string_format(("%08X"), address).c_str();
			break;
		case 2: //Relation:
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
			auto size = m_line.size();
			size.rheight() += 15;
			size.rwidth() += 15;
			return size;
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

			//Draw current instruction m_arrow and m_breakpoint icon
			if(isBreakpoint && isPC)
			{
				return m_breakpoint_arrow;
			}
			//Draw m_breakpoint icon
			if(isBreakpoint)
			{
				return m_breakpoint;
			}
			//Draw current instruction m_arrow
			if(isPC)
			{
				return m_arrow;
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
					return m_start_line;
				}
				else if(address == sub->end)
				{
					return m_end_line;
				}
				else
				{
					return m_line;
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
	return "";
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
