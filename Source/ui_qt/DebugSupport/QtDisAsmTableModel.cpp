#include <QPainter>

#include "QtDisAsmTableModel.h"
#include "string_cast.h"
#include "string_format.h"
#include "lexical_cast_ex.h"

CQtDisAsmTableModel::CQtDisAsmTableModel(QObject* parent, CVirtualMachine& virtualMachine, CMIPS* context, std::vector<std::string> headers)
    : QAbstractTableModel(parent)
	, m_virtualMachine(virtualMachine)
    , m_ctx(context)
    , m_instructionSize(4)
{
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
		start_path.moveTo(75 / 3, 150 / 3);
		start_path.lineTo(75 / 3, 75 / 3);
		start_path.lineTo(150 / 3, 75 / 3);

		QPainter painter(&start_line);
		painter.setPen(pen);
		painter.drawPath(start_path);
	}
	{
		end_line.fill(Qt::transparent);

		QPainterPath end_path;
		end_path.moveTo(75 / 3, 0 / 3);
		end_path.lineTo(75 / 3, 75 / 3);
		end_path.lineTo(150 / 3, 75 / 3);

		QPainter painter(&end_line);
		painter.setPen(pen);
		painter.drawPath(end_path);
	}
	{
		line.fill(Qt::transparent);

		QPainterPath line_path;
		line_path.moveTo(75 / 3, 0 / 3);
		line_path.lineTo(75 / 3, 150 / 3);

		QPainter painter(&line);
		painter.setPen(pen);
		painter.drawPath(line_path);
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
		// 		//Draw breakpoint icon
		// 		if(m_ctx->m_breakpoints.find(address) != std::end(m_ctx->m_breakpoints))
		// 		{
		// 			auto breakpointSrcRect = Framework::Win32::MakeRectPositionSize(0, 0, 15, 15);
		// 			auto breakpointDstRect = Framework::Win32::PointsToPixels(breakpointSrcRect).CenterInside(iconArea);
		// 			DrawMaskedBitmap(hDC, breakpointDstRect, m_breakpointBitmap, m_breakpointMaskBitmap, breakpointSrcRect);
		// 		}

		// 		//Draw current instruction arrow
		// 		if(m_virtualMachine.GetStatus() != CVirtualMachine::RUNNING)
		// 		{
		// 			if(address == m_ctx->m_State.nPC)
		// 			{
		// 				auto arrowSrcRect = Framework::Win32::MakeRectPositionSize(0, 0, 13, 13);
		// 				auto arrowDstRect = Framework::Win32::PointsToPixels(arrowSrcRect).CenterInside(iconArea);
		// 				DrawMaskedBitmap(hDC, arrowDstRect, m_arrowBitmap, m_arrowMaskBitmap, arrowSrcRect);
		// 			}
		// 		}
				return QVariant();
			break;
			case 1://Address:
				return string_format(("%08X"), address).c_str();
			break;
			case 2://Relation:
				return QVariant();
			break;
			case 3://Instruction :
				{
					uint32 data = GetInstruction(address);
					return lexical_cast_hex<std::string>(data, 8).c_str();
				}
			break;
			case 4://Instruction Mno:
				{
					uint32 data = GetInstruction(address);
					char disAsm[256];
					m_ctx->m_pArch->GetInstructionMnemonic(m_ctx, address, data, disAsm, 256);
					return disAsm;
				}
			break;
			case 5://Instruction Oper:
				{
					uint32 data = GetInstruction(address);
					char disAsm[256];
					m_ctx->m_pArch->GetInstructionOperands(m_ctx, address, data, disAsm, 256);
					return disAsm;
				}
			break;
			// case 6://Name:

			// break;
			// case 7://...

			// break;
		}
	}
	if(role == Qt::SizeHintRole)
	{
		if(index.column() == 2)
		{
			return line.size();
		}
	}
	if(role == Qt::DecorationRole)
	{
		if(index.column() == 2)
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

bool CQtDisAsmTableModel::addItem(std::vector<std::string> data)
{
	if(data.size() != m_headers.size())
		return false;

	emit QAbstractTableModel::beginInsertRows(QModelIndex(), rowCount(), rowCount());
	m_data.push_back(data);
	emit QAbstractTableModel::endInsertRows();
	return true;
}

std::string CQtDisAsmTableModel::getItem(const QModelIndex& index)
{
	if(m_data.size() > index.row())
	{
		auto data = m_data.at(index.row());
		if(data.size() > index.column())
			return data.at(index.column());
	}
	return nullptr;
}

void CQtDisAsmTableModel::clear()
{
	emit QAbstractTableModel::beginResetModel();
	m_data.clear();
	emit QAbstractTableModel::endResetModel();
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