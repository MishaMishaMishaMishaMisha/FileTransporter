

#include "ServerGUI.h"


// ======= start gui =======
class FTServer : public wxApp
{
public:
	bool OnInit() override
	{
		ServerGUI* frame = new ServerGUI("Server GUI");
		frame->Show(true);

		return true;
	}
};

wxIMPLEMENT_APP(FTServer);