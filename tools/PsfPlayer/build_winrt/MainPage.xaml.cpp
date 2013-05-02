#include "pch.h"
#include "MainPage.xaml.h"
#include "string_cast.h"
#include "PsfLoader.h"
#include "win32_ui/SH_XAudio2.h"
#include "winrt/StorageFileStream.h"
#include <ppltasks.h>

using namespace PsfPlayer;

using namespace Platform;
using namespace Windows::Foundation;
using namespace Windows::Foundation::Collections;
using namespace Windows::Storage;
using namespace Windows::Storage::Pickers;
using namespace Windows::UI::Xaml;
using namespace Windows::UI::Xaml::Controls;
using namespace Windows::UI::Xaml::Controls::Primitives;
using namespace Windows::UI::Xaml::Data;
using namespace Windows::UI::Xaml::Input;
using namespace Windows::UI::Xaml::Media;
using namespace Windows::UI::Xaml::Navigation;

MainPage::MainPage()
{
	PlaylistItems = ref new Platform::Collections::Vector<PlaylistItemAdapter^>();

	m_playlist.OnItemInsert.connect(
		[&] (const CPlaylist::ITEM& item)
		{
			auto newAdapter = ref new PlaylistItemAdapter();
			newAdapter->Title = ref new String(string_cast<std::wstring>(item.title).c_str());
			newAdapter->Length = "2:00";
			PlaylistItems->Append(newAdapter);
		}
	);

	m_playlist.OnItemsClear.connect(
		[&] ()
		{
			PlaylistItems->Clear();
		}
	);

	m_virtualMachine.SetSpuHandler(&CSH_XAudio2::HandlerFactory);

	InitializeComponent();
	playlistListBox->ItemsSource = PlaylistItems;
}

void MainPage::OnNavigatedTo(NavigationEventArgs^)
{

}

void PsfPlayer::MainPage::openFileButton_Click(Platform::Object^ sender, Windows::UI::Xaml::RoutedEventArgs^ e)
{
	auto filePicker = ref new FileOpenPicker();
	filePicker->FileTypeFilter->Append(".zip");
	filePicker->ViewMode = PickerViewMode::List;

	auto pickTask = concurrency::create_task(filePicker->PickSingleFileAsync());
	pickTask.then(
		[&] (StorageFile^ result)
		{
			if(result != nullptr)
			{
				m_playlist.Clear();

				unsigned int archiveId = m_playlist.InsertArchive(result->Path->Data());
				auto archive = CPsfArchive::CreateFromPath(result->Path->Data());

				for(const auto& fileInfo : archive->GetFiles())
				{
					auto filePath = boost::filesystem::path(fileInfo.name);
					auto fileExtension = filePath.extension().string();
					if(!fileExtension.empty() && CPlaylist::IsLoadableExtension(fileExtension.c_str() + 1))
					{
						CPlaylist::ITEM newItem;
						newItem.path		= filePath.wstring();
						newItem.title		= newItem.path;
						newItem.length		= 0;
						newItem.archiveId	= archiveId;
						unsigned int itemId = m_playlist.InsertItem(newItem);
					}
				}
			}
		}
	);

//		m_virtualMachine.Pause();
//		m_virtualMachine.Reset();

//		m_currentArchive = CPsfArchive::CreateFromPath(result->Path->Data());
//		auto fileInfo = *m_currentArchive->GetFilesBegin();
//		CPsfLoader::LoadPsf(m_virtualMachine, fileInfo.name, result->Path->Data());

//		m_virtualMachine.Resume();
}
