#pragma once

#include "MainPage.g.h"
#include "PsfVm.h"

namespace PsfPlayer
{
	public ref class MainPage sealed
	{
	public:
						MainPage();

	protected:
		virtual void	OnNavigatedTo(Windows::UI::Xaml::Navigation::NavigationEventArgs^ e) override;

	private:
		CPsfVm			m_virtualMachine;
	};
}
