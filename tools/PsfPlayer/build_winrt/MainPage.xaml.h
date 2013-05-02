#pragma once

#include "MainPage.g.h"
#include "PsfVm.h"
#include "Playlist.h"
#include "winrt_ui/PlaylistItemAdapter.h"

namespace PsfPlayer
{
	public ref class MainPage sealed
	{
	public:
		typedef Windows::Foundation::Collections::IVector<PlaylistItemAdapter^>^ PlaylistItemArray; 

										MainPage();

		property PlaylistItemArray		PlaylistItems;

	protected:
		virtual void					OnNavigatedTo(Windows::UI::Xaml::Navigation::NavigationEventArgs^ e) override;

	private:
		void							openFileButton_Click(Platform::Object^ sender, Windows::UI::Xaml::RoutedEventArgs^ e);

		CPsfVm							m_virtualMachine;
		CPlaylist						m_playlist;
	};
}
