#include "pch.h"
#include "MainPage.xaml.h"
#include "StdStreamUtils.h"
#include "PsfArchive.h"
#include "PsfLoader.h"
#include "win32_ui/SH_XAudio2.h"

using namespace PsfPlayer;

using namespace Platform;
using namespace Windows::Foundation;
using namespace Windows::Foundation::Collections;
using namespace Windows::UI::Xaml;
using namespace Windows::UI::Xaml::Controls;
using namespace Windows::UI::Xaml::Controls::Primitives;
using namespace Windows::UI::Xaml::Data;
using namespace Windows::UI::Xaml::Input;
using namespace Windows::UI::Xaml::Media;
using namespace Windows::UI::Xaml::Navigation;

MainPage::MainPage()
{
	InitializeComponent();

	m_virtualMachine.SetSpuHandler(&CSH_XAudio2::HandlerFactory);
}

void MainPage::OnNavigatedTo(NavigationEventArgs^)
{
	auto installedLocation = Windows::ApplicationModel::Package::Current->InstalledLocation;
	installedLocation->Path;
	auto file = installedLocation->GetFileAsync("ff7-psf.zip");
	file->Completed = ref new AsyncOperationCompletedHandler<Windows::Storage::StorageFile^>(
		[&] (IAsyncOperation<Windows::Storage::StorageFile^>^ operation, AsyncStatus operationStatus)
		{
			if(operationStatus == AsyncStatus::Completed)
			{
				auto storageFile = operation->GetResults();
				CPsfLoader::LoadPsf(m_virtualMachine, "ff7-psf/FF7 101 The Prelude.minipsf", storageFile->Path->Data());
				m_virtualMachine.Resume();
			}
		});
}
