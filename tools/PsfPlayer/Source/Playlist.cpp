#include <boost/scoped_ptr.hpp>
#include <boost/filesystem/operations.hpp>
#include <boost/filesystem/path.hpp>
#include "StdStream.h"
#include "path_uncomplete.h"
#include "string_cast.h"
#include "xml/Parser.h"
#include "xml/Writer.h"
#include "xml/Utils.h"
#include "Utf8.h"
#include "Playlist.h"

#define PLAYLIST_NODE_TAG               "Playlist"
#define PLAYLIST_ITEM_NODE_TAG          "Item"
#define PLAYLIST_ITEM_PATH_ATTRIBUTE    ("Path")
#define PLAYLIST_ITEM_TITLE_ATTRIBUTE   ("Title")
#define PLAYLIST_ITEM_LENGTH_ATTRIBUTE  ("Length")

using namespace Framework;

CPlaylist::CPlaylist()
{

}

CPlaylist::~CPlaylist()
{

}

const CPlaylist::ITEM& CPlaylist::GetItem(unsigned int index) const
{
    assert(index < m_items.size());
    if(index >= m_items.size())
    {
        throw std::runtime_error("Invalid item index.");
    }
    return m_items[index];
}

unsigned int CPlaylist::GetItemCount() const
{
    return m_items.size();
}

void CPlaylist::PopulateItemFromTags(ITEM& item, const CPsfTags& tags)
{
    item.title      = tags.GetTagValue("title");
    item.length     = CPsfTags::ConvertTimeString(tags.GetTagValue("length").c_str());
}

void CPlaylist::InsertItem(const ITEM& item)
{
    m_items.push_back(item);
    OnItemInsert(item);
}

void CPlaylist::DeleteItem(unsigned int index)
{
    assert(index < m_items.size());
    if(index >= m_items.size())
    {
        throw std::runtime_error("Invalid item index.");
    }
    m_items.erase(m_items.begin() + index);
    OnItemDelete(index);
}

void CPlaylist::UpdateItem(unsigned int index, const CPlaylist::ITEM& item)
{
    assert(index < m_items.size());
    if(index >= m_items.size())
    {
        throw std::runtime_error("Invalid item index.");
    }
    m_items[index] = item;
    OnItemUpdate(index, item);
}

void CPlaylist::Clear()
{
    OnItemsClear();
    m_items.clear();
}

void CPlaylist::Read(const char* inputPathString)
{
    Clear();

    CStdStream stream(inputPathString, "rb");
    boost::scoped_ptr<Xml::CNode> document(Xml::CParser::ParseDocument(&stream));
    if(!document)
    {
        throw std::runtime_error("Couldn't parse document.");
    }
    boost::filesystem::path inputPath(inputPathString);
    inputPath = inputPath.parent_path();

    Xml::CNode::NodeList items = document->SelectNodes(PLAYLIST_NODE_TAG "/" PLAYLIST_ITEM_NODE_TAG);
    for(Xml::CNode::NodeIterator nodeIterator(items.begin()); nodeIterator != items.end(); nodeIterator++)
    {
        Xml::CNode* itemNode = (*nodeIterator);
        boost::filesystem::path itemPath = itemNode->GetAttribute(PLAYLIST_ITEM_PATH_ATTRIBUTE);
        const char* title = itemNode->GetAttribute(PLAYLIST_ITEM_TITLE_ATTRIBUTE);
        const char* length = itemNode->GetAttribute(PLAYLIST_ITEM_LENGTH_ATTRIBUTE);

        if(!itemPath.is_complete())
        {
            itemPath = inputPath / itemPath;
        }

        ITEM item;
        item.path = itemPath.string().c_str();
        if(title != NULL)
        {
            item.title = string_cast<std::wstring>(title);
        }

        if(length != NULL)
        {
            item.length = atoi(length);
        }
        else
        {
            item.length = 0;
        }

        InsertItem(item);
    }
}

void CPlaylist::Write(const char* outputPathString)
{
    CStdStream stream(outputPathString, "wb");

    boost::scoped_ptr<Xml::CNode> document(new Xml::CNode());
    Xml::CNode* playlistNode = document->InsertNode(new Xml::CNode(PLAYLIST_NODE_TAG, true));

    boost::filesystem::path outputPath(outputPathString);
    outputPath = outputPath.parent_path();

    for(ItemIterator itemIterator(m_items.begin());
        itemIterator != m_items.end(); itemIterator++)
    {
        const ITEM& item(*itemIterator);

        boost::filesystem::path itemPath(item.path);
        boost::filesystem::path itemRelativePath(naive_uncomplete(itemPath, outputPath));

        Xml::CNode* itemNode = playlistNode->InsertNode(new Xml::CNode(PLAYLIST_ITEM_NODE_TAG, true));
        itemNode->InsertAttribute(PLAYLIST_ITEM_PATH_ATTRIBUTE, itemRelativePath.string().c_str());
        itemNode->InsertAttribute(PLAYLIST_ITEM_TITLE_ATTRIBUTE, Utf8::ConvertTo(item.title).c_str());
        itemNode->InsertAttribute(Xml::CreateAttributeIntValue(PLAYLIST_ITEM_LENGTH_ATTRIBUTE, item.length));
    }

    Xml::CWriter::WriteDocument(&stream, document.get());
}
