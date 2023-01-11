#include <QHeaderView>
#include "string_format.h"
#include "KernelObjectListView.h"
#include "DebugUtils.h"
#include "QtDialogListWidget.h"

CKernelObjectListView::CKernelObjectListView(QWidget* parent)
    : QTableView(parent)
{
	setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Expanding);
	setSelectionBehavior(QAbstractItemView::SelectRows);
	connect(this, &QTableView::doubleClicked, this, &CKernelObjectListView::tableDoubleClick);
}

void CKernelObjectListView::HandleMachineStateChange()
{
	Update();
}

void CKernelObjectListView::SetContext(CMIPS* context, CBiosDebugInfoProvider* biosDebugInfoProvider)
{
	m_context = context;
	m_biosDebugInfoProvider = biosDebugInfoProvider;
	SetObjectType(BIOS_DEBUG_OBJECT_TYPE_NULL);
	Update();
}

void CKernelObjectListView::SetObjectType(uint32 objectType)
{
	m_objectType = objectType;

	m_schema = m_biosDebugInfoProvider ? m_biosDebugInfoProvider->GetBiosObjectsDebugInfo() : BiosDebugObjectInfoMap();
	if(!m_schema.empty() && (m_objectType != BIOS_DEBUG_OBJECT_TYPE_NULL))
	{
		auto objectType = m_schema[m_objectType];
		assert(!objectType.fields.empty());
		assert(objectType.fields[0].HasAttribute(BIOS_DEBUG_OBJECT_FIELD_ATTRIBUTE::IDENTIFIER));

		std::vector<std::string> headers;
		for(const auto& field : objectType.fields)
		{
			headers.push_back(field.name);
		}

		m_model = new CQtGenericTableModel(this, headers);
		setModel(m_model);

		auto header = horizontalHeader();
		for(int i = 0; i < objectType.fields.size(); i++)
		{
			const auto& field = objectType.fields[i];
			header->setSectionHidden(i, field.HasAttribute(BIOS_DEBUG_OBJECT_FIELD_ATTRIBUTE::HIDDEN));
			if(
			    field.HasAttribute(BIOS_DEBUG_OBJECT_FIELD_ATTRIBUTE::LOCATION) ||
			    field.HasAttribute(BIOS_DEBUG_OBJECT_FIELD_ATTRIBUTE::POSSIBLE_STR_POINTER))
			{
				header->setSectionResizeMode(i, QHeaderView::Stretch);
			}
			else
			{
				header->setSectionResizeMode(i, QHeaderView::ResizeToContents);
			}
		}
		verticalHeader()->setSectionResizeMode(QHeaderView::ResizeToContents);
		verticalHeader()->hide();

		OnObjectTypeChanged(objectType.name.c_str());
	}
	else
	{
		setModel(nullptr);
		OnObjectTypeChanged("No Object Type");
	}

	Update();
}

void CKernelObjectListView::Update()
{
	if(m_model)
	{
		m_model->clear();
	}

	if(!m_biosDebugInfoProvider || (m_objectType == BIOS_DEBUG_OBJECT_TYPE_NULL) || (m_schema.empty())) return;

	auto moduleInfos = m_biosDebugInfoProvider->GetModulesDebugInfo();

	auto objects = m_biosDebugInfoProvider->GetBiosObjects(m_objectType);
	auto objectType = m_schema[m_objectType];
	for(const auto& object : objects)
	{
		assert(object.fields.size() == objectType.fields.size());
		std::vector<std::string> data;
		for(int fieldIdx = 0; fieldIdx < object.fields.size(); fieldIdx++)
		{
			const auto& fieldType = objectType.fields[fieldIdx];
			const auto& field = object.fields[fieldIdx];
			if(auto intValue = std::get_if<uint32>(&field))
			{
				assert(fieldType.type == BIOS_DEBUG_OBJECT_FIELD_TYPE::UINT32);
				if(fieldType.HasAttribute(BIOS_DEBUG_OBJECT_FIELD_ATTRIBUTE::TEXT_ADDRESS))
				{
					auto locationString = DebugUtils::PrintAddressLocation(*intValue, m_context, moduleInfos);
					data.push_back(locationString);
				}
				else if(fieldType.HasAttribute(BIOS_DEBUG_OBJECT_FIELD_ATTRIBUTE::DATA_ADDRESS))
				{
					data.push_back(string_format("0x%08X", *intValue));
				}
				else if(fieldType.HasAttribute(BIOS_DEBUG_OBJECT_FIELD_ATTRIBUTE::POSSIBLE_STR_POINTER))
				{
					std::string value;
					if(CMIPSAnalysis::TryGetStringAtAddress(m_context, *intValue, value))
					{
						data.push_back(string_format("0x%08X (%s)", *intValue, value.c_str()));
					}
					else
					{
						data.push_back(string_format("0x%08X", *intValue));
					}
				}
				else
				{
					data.push_back(string_format("%u", *intValue));
				}
			}
			else if(auto strValue = std::get_if<std::string>(&field))
			{
				assert(fieldType.type == BIOS_DEBUG_OBJECT_FIELD_TYPE::STRING);
				data.push_back(*strValue);
			}
		}
		bool added = m_model->addItem(data);
		assert(added);
	}
}

void CKernelObjectListView::tableDoubleClick(const QModelIndex& indexRow)
{
	auto objectType = m_schema[m_objectType];
	auto action = objectType.selectionAction;
	if(action == BIOS_DEBUG_OBJECT_ACTION::NONE)
	{
		return;
	}

	//Assuming col 0 has id (verified in SetObjectType)
	auto fieldType = objectType.fields[0];
	auto index = m_model->index(indexRow.row(), 0);
	uint32 objectId = 0;

	//TODO: Find a better way to fetch the id/key from the indexRow.
	auto idCellValue = m_model->getItem(index);
	if(fieldType.HasAttribute(BIOS_DEBUG_OBJECT_FIELD_ATTRIBUTE::DATA_ADDRESS))
	{
		objectId = std::strtoul(idCellValue.c_str(), nullptr, 16);
	}
	else
	{
		objectId = std::strtoul(idCellValue.c_str(), nullptr, 10);
	}

	auto objects = m_biosDebugInfoProvider->GetBiosObjects(m_objectType);

	auto objectIterator = std::find_if(std::begin(objects), std::end(objects),
	                                   [&](const BIOS_DEBUG_OBJECT& object) {
		                                   assert(!object.fields.empty());
		                                   if(auto id = std::get_if<uint32>(&object.fields[0]))
		                                   {
			                                   return (*id) == objectId;
		                                   }
		                                   return false;
	                                   });
	if(objectIterator == std::end(objects)) return;

	const auto& object(*objectIterator);

	switch(action)
	{
	case BIOS_DEBUG_OBJECT_ACTION::SHOW_LOCATION:
	{
		int locationFieldIndex = objectType.FindFieldWithAttribute(BIOS_DEBUG_OBJECT_FIELD_ATTRIBUTE::LOCATION);
		assert(locationFieldIndex != -1);
		auto location = std::get_if<uint32>(&object.fields[locationFieldIndex]);
		assert(location);
		OnGotoAddress(*location);
	}
	break;
	case BIOS_DEBUG_OBJECT_ACTION::SHOW_STACK_OR_LOCATION:
	{
		int pcFieldIndex = objectType.FindFieldWithAttribute(BIOS_DEBUG_OBJECT_FIELD_ATTRIBUTE::LOCATION);
		int spFieldIndex = objectType.FindFieldWithAttribute(BIOS_DEBUG_OBJECT_FIELD_ATTRIBUTE::STACK_POINTER);
		int raFieldIndex = objectType.FindFieldWithAttribute(BIOS_DEBUG_OBJECT_FIELD_ATTRIBUTE::RETURN_ADDRESS);

		assert(pcFieldIndex != -1);
		assert(spFieldIndex != -1);
		assert(raFieldIndex != -1);

		auto pc = std::get_if<uint32>(&object.fields[pcFieldIndex]);
		auto sp = std::get_if<uint32>(&object.fields[spFieldIndex]);
		auto ra = std::get_if<uint32>(&object.fields[raFieldIndex]);

		assert(pc && sp && ra);

		auto callStackItems = CMIPSAnalysis::GetCallStack(m_context, *pc, *sp, *ra);
		if(callStackItems.size() <= 1)
		{
			OnGotoAddress(*pc);
		}
		else
		{
			std::map<std::string, uint32> addrMap;
			QtDialogListWidget dialog(this);
			for(auto itemIterator(std::begin(callStackItems));
			    itemIterator != std::end(callStackItems); itemIterator++)
			{
				const auto& item(*itemIterator);
				std::string locationString = DebugUtils::PrintAddressLocation(item, m_context, m_biosDebugInfoProvider->GetModulesDebugInfo());
				dialog.addItem(locationString);
				addrMap.insert({locationString, item});
			}

			dialog.exec();
			auto locationString = dialog.getResult();
			if(!locationString.empty())
			{
				auto address = addrMap.at(locationString);
				OnGotoAddress(address);
			}
		}
	}
	break;
	default:
		assert(false);
		break;
	}
}
