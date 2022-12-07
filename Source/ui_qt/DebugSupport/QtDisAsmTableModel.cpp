#include <QGuiApplication>
#include <QPalette>
#include <QPainter>
#include <QPainterPath>

#include "QtDisAsmTableModel.h"
#include "string_cast.h"
#include "string_format.h"
#include "lexical_cast_ex.h"

CQtDisAsmTableModel::CQtDisAsmTableModel(QTableView* parent, CVirtualMachine& virtualMachine, CMIPS* context, DISASM_TYPE disAsmType)
    : QAbstractTableModel(parent)
    , m_virtualMachine(virtualMachine)
    , m_ctx(context)
    , m_instructionSize(4)
    , m_disAsmType(disAsmType)
    , m_maps(m_ctx->m_pMemoryMap->GetInstructionMaps())
{
	m_headers = {"S", "Address", "R", "Instr", "I-Mn", "I-Op", "Target/Comments"};

	auto target_comment_column_index = ((m_disAsmType == CQtDisAsmTableModel::DISASM_STANDARD) ? 3 : 5) + 3;
	parent->setItemDelegateForColumn(target_comment_column_index, new TableColumnDelegateTargetComment(parent));

	for(const auto& map : m_maps)
	{
		m_memSize += (map.nEnd + 1) - map.nStart;
	}
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

int CQtDisAsmTableModel::rowCount(const QModelIndex& /*parent*/) const
{
	return m_memSize / m_instructionSize;
}

int CQtDisAsmTableModel::columnCount(const QModelIndex& /*parent*/) const
{
	return m_headers.size();
}

QVariant CQtDisAsmTableModel::data(const QModelIndex& index, int role) const
{
	// "S", "Address", "R", "I-Mn", "I-Op", "Name",
	uint32 address = TranslateModelIndexToAddress(index);
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
	if(role == Qt::UserRole)
	{
		auto size = (m_disAsmType == DISASM_TYPE::DISASM_STANDARD) ? 3 : 5;
		auto subindex = index.column() - 3;
		if(subindex == size)
			return m_ctx->m_Comments.Find(address);
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
	if(role == Qt::BackgroundRole)
	{
		auto map_index = 0;
		for(const auto& map : m_maps)
		{
			if(address <= map.nEnd)
				break;
			++map_index;
		}
		QColor background;
		if(map_index % 2 == 0)
		{
			background = QColor(QPalette().base().color());
		}
		else
		{
			background = QColor(QPalette().alternateBase().color());
		}

		QBrush brush(background);
		return brush;
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
	emit QAbstractTableModel::beginResetModel();
	emit QAbstractTableModel::endResetModel();
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
		m_ctx->m_pArch->GetInstructionMnemonic(m_ctx, address, instruction, disAsm, 256);
		return disAsm;
	}
	case 2:
	{
		char disAsm[256];
		m_ctx->m_pArch->GetInstructionOperands(m_ctx, address, instruction, disAsm, 256);
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
			if(effAddr != MIPS_INVALID_PC)
			{
				const char* tag = m_ctx->m_Functions.Find(effAddr);
				if(tag != nullptr)
				{
					disAsm += ("-> ");
					disAsm += tag;
					commentDrawn = true;
				}
			}
		}
	}
	return disAsm.c_str();
}

uint32 CQtDisAsmTableModel::TranslateAddress(uint32 address) const
{
	uint32 tAddress = address;
	for(auto itr = m_maps.rbegin(); itr != m_maps.rend(); ++itr)
	{
		auto map = *itr;
		if(tAddress >= map.nStart && tAddress < map.nEnd)
			break;

		if(map.nStart > tAddress)
			continue;

		assert(itr != m_maps.rbegin());
		--itr;
		auto map2 = *itr;

		tAddress += map2.nStart;
		tAddress -= map.nEnd + 1;
		break;
	}

	return tAddress;
}

uint32 CQtDisAsmTableModel::TranslateModelIndexToAddress(const QModelIndex& index) const
{
	return TranslateAddress(index.row() * m_instructionSize);
}

const QModelIndex CQtDisAsmTableModel::TranslateAddressToModelIndex(uint32 address) const
{
	uint32 tAddress = address;
	auto itr = m_maps.rbegin();
	for(; itr != m_maps.rend(); ++itr)
	{
		auto map = *itr;
		if(tAddress >= map.nStart && tAddress < map.nEnd)
		{
			tAddress -= map.nStart;
			++itr;
			break;
		}
	}

	for(; itr != m_maps.rend(); ++itr)
	{
		auto map = *itr;
		tAddress += (map.nEnd + 1) - map.nStart;
	}

	return index(tAddress / m_instructionSize, 0);
}

int CQtDisAsmTableModel::GetLinePixMapWidth() const
{
	return m_line.width();
}

TableColumnDelegateTargetComment::TableColumnDelegateTargetComment(QObject* parent)
    : QStyledItemDelegate(parent)
{
}

void TableColumnDelegateTargetComment::paint(QPainter* painter, const QStyleOptionViewItem& option, const QModelIndex& index) const
{
	QString target = index.data(Qt::DisplayRole).toString();
	QString comment = index.data(Qt::UserRole).toString();
	if(!target.isEmpty() || !comment.isEmpty())
	{
		if(!comment.isEmpty())
		{
			comment = ";" + comment;
		}

		painter->save();
		if((option.state & QStyle::State_Selected) && (option.state & QStyle::State_Active))
		{
			painter->setPen(Qt::white);
			painter->fillRect(option.rect, option.palette.highlight());
		}
		else if((option.state & QStyle::State_Selected) && !(option.state & QStyle::State_HasFocus))
		{
			painter->fillRect(option.rect, option.palette.window());
		}

		const QString html = QString("<html><target>%1</target> <comment>%2</comment></html>").arg(target).arg(comment);
		QTextDocument doc;
		doc.setTextWidth(option.rect.width());
		doc.setDefaultStyleSheet("target {color : blue;} comment {color : red;}");
		doc.setHtml(html);

		QRect clip(0, 0, option.rect.width(), option.rect.height());
		painter->translate(option.rect.left(), option.rect.top());
		doc.drawContents(painter, clip);
		painter->restore();
		return;
	}
	QStyledItemDelegate::paint(painter, option, index);
}
