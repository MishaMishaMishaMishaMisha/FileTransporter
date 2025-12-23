
#include "ClientGUI.h"


// ======= start gui =======
class FTClient : public wxApp
{
public:
	bool OnInit() override
	{
		ClientGUI* frame = new ClientGUI("Client GUI");
		frame->Show(true);

		frame->CallAfter(&ClientGUI::LaunchClient);

		return true;
	}
};

wxIMPLEMENT_APP(FTClient);