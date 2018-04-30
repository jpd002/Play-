#pragma once

namespace PsfPlayer
{
	[Windows::UI::Xaml::Data::Bindable] public ref class PlaylistItemAdapter sealed
	{
	public:
		PlaylistItemAdapter();

		property Platform::String ^ Title;
		property Platform::String ^ Length;
		property unsigned int ItemId;
		property unsigned int ArchiveId;
	};
}
