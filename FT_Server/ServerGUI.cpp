
#include "ServerGUI.h"

#include <fstream>



wxBEGIN_EVENT_TABLE(ServerGUI, wxFrame)
EVT_BUTTON(ID_BTN_SETTINGS, ServerGUI::OnSettings)
EVT_BUTTON(ID_BTN_CLEAR_STORAGE, ServerGUI::OnClearStorage)
EVT_BUTTON(ID_BTN_START, ServerGUI::OnStartServer)
EVT_BUTTON(ID_BTN_STOP, ServerGUI::OnStopServer)
wxEND_EVENT_TABLE()


ServerGUI::ServerGUI(const wxString& title)
	: wxFrame(nullptr, wxID_ANY, title, wxDefaultPosition, wxSize(600, 450)), 
	server(port),
	timer_checkConnections(this, wxID_HIGHEST + 1),
	timer_cleanTempFiles(this, wxID_HIGHEST + 2),
	timer_cleanOldFiles(this, wxID_HIGHEST + 3),
	timer_updateLabels(this, wxID_HIGHEST + 4)
{

	wxPanel* panel = new wxPanel(this, wxID_ANY);

	// left panel
	wxBoxSizer* leftSizer = new wxBoxSizer(wxVERTICAL);

	btnSettings = new wxButton(panel, ID_BTN_SETTINGS, "Settings");
	btnClearStorage = new wxButton(panel, ID_BTN_CLEAR_STORAGE, "Clear storage");
	btnStartServer = new wxButton(panel, ID_BTN_START, "Start server");
	btnStopServer = new wxButton(panel, ID_BTN_STOP, "Stop server");

	leftSizer->Add(btnSettings, 0, wxEXPAND | wxALL, 5);
	leftSizer->Add(btnClearStorage, 0, wxEXPAND | wxALL, 5);
	leftSizer->AddSpacer(10);
	leftSizer->Add(btnStartServer, 0, wxEXPAND | wxALL, 5);
	leftSizer->Add(btnStopServer, 0, wxEXPAND | wxALL, 5);
	leftSizer->AddStretchSpacer(1);

	// right panel
	// labels
	// two rows - static and mutable labels
	wxFlexGridSizer* gridSizer = new wxFlexGridSizer(2, 10, 10);

	// static (titles)
	wxStaticText* label_serverStatus = new wxStaticText(panel, wxID_ANY, "Server staus: ");
	wxStaticText* label_freeSpace = new wxStaticText(panel, wxID_ANY, "Free space: ");
	wxStaticText* label_reservedSpace = new wxStaticText(panel, wxID_ANY, "Reserved space: ");
	wxStaticText* label_files = new wxStaticText(panel, wxID_ANY, "Amount of files: ");
	wxStaticText* label_tempFiles = new wxStaticText(panel, wxID_ANY, "Amount of temp files: ");
	wxStaticText* label_users = new wxStaticText(panel, wxID_ANY, "Connected users: ");
	wxStaticText* label_nextCleanFiles = new wxStaticText(panel, wxID_ANY, "Next clean old files: ");
	wxStaticText* label_nextCleanTemp = new wxStaticText(panel, wxID_ANY, "Next clean temp files: ");

	// values
	lbl_value_ServerStatus = new wxStaticText(panel, wxID_ANY, "STOPPED");
	lbl_value_ServerStatus->SetForegroundColour(*wxRED);
	lbl_value_ServerStatus->Refresh();

	lbl_value_FreeSpace = new wxStaticText(panel, wxID_ANY, "...");
	lbl_value_ReservedSpace = new wxStaticText(panel, wxID_ANY, "...");
	lbl_value_Files = new wxStaticText(panel, wxID_ANY, "...");
	lbl_value_TempFiles = new wxStaticText(panel, wxID_ANY, "...");
	lbl_value_Users = new wxStaticText(panel, wxID_ANY, "...");
	lbl_value_NextCleanFiles = new wxStaticText(panel, wxID_ANY, "...");
	lbl_value_NextCleanTemp = new wxStaticText(panel, wxID_ANY, "...");


	gridSizer->Add(label_serverStatus);
	gridSizer->Add(lbl_value_ServerStatus, 1, wxEXPAND);
	gridSizer->Add(label_freeSpace);
	gridSizer->Add(lbl_value_FreeSpace, 1, wxEXPAND);
	gridSizer->Add(label_reservedSpace);
	gridSizer->Add(lbl_value_ReservedSpace, 1, wxEXPAND);
	gridSizer->Add(label_files);
	gridSizer->Add(lbl_value_Files, 1, wxEXPAND);
	gridSizer->Add(label_tempFiles);
	gridSizer->Add(lbl_value_TempFiles, 1, wxEXPAND);
	gridSizer->Add(label_users);
	gridSizer->Add(lbl_value_Users, 1, wxEXPAND);
	gridSizer->Add(label_nextCleanFiles);
	gridSizer->Add(lbl_value_NextCleanFiles, 1, wxEXPAND);
	gridSizer->Add(label_nextCleanTemp);
	gridSizer->Add(lbl_value_NextCleanTemp, 1, wxEXPAND);

	gridSizer->AddGrowableCol(1, 1);

	// layout
	wxBoxSizer* topSizer = new wxBoxSizer(wxHORIZONTAL);
	topSizer->Add(leftSizer, 0, wxEXPAND | wxALL, 10);
	topSizer->Add(gridSizer, 1, wxEXPAND | wxALL, 10);

	// console for output info
	consoleOutput = new wxTextCtrl(panel, wxID_ANY, "", wxDefaultPosition, wxSize(-1, 150),
		wxTE_MULTILINE | wxTE_READONLY);

	wxBoxSizer* mainSizer = new wxBoxSizer(wxVERTICAL);

	mainSizer->Add(topSizer, 1, wxEXPAND | wxALL, 10);
	mainSizer->AddSpacer(15);
	mainSizer->Add(consoleOutput, 0, wxEXPAND | wxALL, 5);

	panel->SetSizer(mainSizer);



	// timers
	Bind(wxEVT_TIMER, &ServerGUI::OnTimer_CheckConnections, this, timer_checkConnections.GetId());
	Bind(wxEVT_TIMER, &ServerGUI::OnTimer_CleanOldFiles, this, timer_cleanOldFiles.GetId());
	Bind(wxEVT_TIMER, &ServerGUI::OnTimer_CleanTempFiles, this, timer_cleanTempFiles.GetId());
	Bind(wxEVT_TIMER, &ServerGUI::OnUpdateLabels, this, timer_updateLabels.GetId());


	SetCallbackFunctions();


	LoadSettings(); // set default settings


	btnSettings->Enable(true);
	btnClearStorage->Enable(true);
	btnStartServer->Enable(true);
	btnStopServer->Enable(false);

}

ServerGUI::~ServerGUI()
{
	StopServer();
}

void ServerGUI::LaunchServer()
{

	running = true;
	thr_serverWork = std::thread([this]()
		{
			this->CallAfter(&ServerGUI::PrintToConsole, "Try to launch server");
			if (server.Start())
			{

				btnSettings->Enable(false);
				btnClearStorage->Enable(false);
				btnStartServer->Enable(false);
				btnStopServer->Enable(true);


				this->CallAfter([this]() 
					{
						lbl_value_ServerStatus->SetLabel("WORKING");
						lbl_value_ServerStatus->SetForegroundColour(*wxGREEN);
						lbl_value_ServerStatus->Refresh();
					});


				this->CallAfter(&ServerGUI::PrintToConsole, "Wait for users...");
				size_t max_messages = 5; // update queue of messages per this value
				while (running)
				{
					server.Update(max_messages, true);
				}

			}

		});

}

void ServerGUI::StopServer()
{
	running = false;
	ft::filehash::IsCancelCalculatingHash.store(true); // if hash calculate right now, cancel it

	server.get_IncomingMsg().wakeUpCV();

	if (thr_serverWork.joinable())
	{
		thr_serverWork.join();
	}
	server.Stop();

	this->CallAfter([this]()
		{
			lbl_value_ServerStatus->SetLabel("STOPPED");
			lbl_value_ServerStatus->SetForegroundColour(*wxRED);
			lbl_value_ServerStatus->Refresh();
		});


	btnSettings->Enable(true);
	btnClearStorage->Enable(true);
	btnStartServer->Enable(true);
	btnStopServer->Enable(false);

}

void ServerGUI::OnTimer_CheckConnections(wxTimerEvent & event)
{
	server.CallCheckConnectionMethod();
}
void ServerGUI::OnTimer_CleanOldFiles(wxTimerEvent& event)
{
	server.CleanStorage();
	nextCleanOldTime = wxDateTime::Now() + wxTimeSpan::Seconds(period_seconds_cleanOldFiles);
}
void ServerGUI::OnTimer_CleanTempFiles(wxTimerEvent& event)
{
	server.CleanTemp();
	nextCleanTempTime = wxDateTime::Now() + wxTimeSpan::Seconds(period_seconds_cleanTempFiles);
}
void ServerGUI::OnUpdateLabels(wxTimerEvent&)
{
	auto updateLabel = [](wxStaticText* label, const wxDateTime& nextTime)
		{
			wxTimeSpan remaining = nextTime - wxDateTime::Now();
			if (remaining.IsPositive())
			{
				int hours = remaining.GetHours();
				int minutes = remaining.GetMinutes() % 60;
				int seconds = remaining.GetSeconds().ToLong() % 60;

				wxString text;
				text.Printf("%02dh %02dm %02ds", hours, minutes, seconds);
				label->SetLabel(text);
			}
			else
			{
				label->SetLabel("Cleaning...");
			}
		};

	updateLabel(lbl_value_NextCleanFiles, nextCleanOldTime);
	updateLabel(lbl_value_NextCleanTemp, nextCleanTempTime);

}


void ServerGUI::PrintToConsole(const wxString& message)
{
	// time
	wxDateTime now = wxDateTime::Now();
	wxString timestamp = now.Format("[%d-%m-%Y %H:%M:%S] : ");

	// print message
	consoleOutput->AppendText(timestamp + message + "\n");
}


// buttons events
void ServerGUI::OnSettings(wxCommandEvent& event)
{
	if (running)
	{
		PrintToConsole("Stop server, please");
		return;
	}

	SettingsDialog dlg(this, settings);

	if (dlg.ShowModal() == wxID_OK)
	{
		dlg.GetSettings(settings);

		SaveSettings();

		uint64_t space = static_cast<uint64_t>(server.getStorageSpace()) * 1024 * 1024 * 1024;
		if (server.getCurrentSpace() > space)
		{
			btnStartServer->Enable(false);
			PrintToConsole("Select storage space bigger than current space");
		}
		else
		{
			btnStartServer->Enable(true);
		}
	}
}

void ServerGUI::OnClearStorage(wxCommandEvent& event)
{
	if (running)
	{
		PrintToConsole("Stop server, please");
		return;
	}

	server.CleanFullStorage();

}

void ServerGUI::OnStartServer(wxCommandEvent& event)
{
	if (!running)
	{
		LaunchServer();

		if (settings.enableCleaning)
		{
			StartTimers();
		}
	}
}

void ServerGUI::OnStopServer(wxCommandEvent& event)
{
	if (running)
	{
		StopServer();

		if (settings.enableCleaning)
		{
			StopTimers();
		}
	}
}

void ServerGUI::SetCallbackFunctions()
{

	server.onMessage = [this](const std::string& msg)
		{
			if (!this)
			{
				return;
			}
			wxString wxs(msg.c_str(), wxConvUTF8);
			this->CallAfter(&ServerGUI::PrintToConsole, wxs);
		};

	server.onUsersCountChanged = [this](const uint32_t users_count)
		{
			if (!this)
			{
				return;
			}
			wxString str = wxString::Format("%u", users_count);
			this->CallAfter([this, str]() {lbl_value_Users->SetLabel(str); });
		};

	server.onCurrentSpaceChanged = [this](const uint64_t cur_space, const uint64_t storage_space)
		{
			if (!this)
			{
				return;
			}
			float GBs_cur = (float)cur_space / (1024 * 1024 * 1024);
			float GBs_stor = (float)storage_space / (1024 * 1024 * 1024);
			wxString str = wxString::Format("%.3f / %.3f", GBs_cur, GBs_stor);
			this->CallAfter([this, str]() {lbl_value_FreeSpace->SetLabel(str); });
		};

	server.onReservedSpaceChanged = [this](const uint64_t reserved_space)
		{
			if (!this)
			{
				return;
			}
			float GBs = (float)reserved_space / (1024 * 1024 * 1024);
			wxString str = wxString::Format("%.3f", GBs);
			this->CallAfter([this, str]() {lbl_value_ReservedSpace->SetLabel(str); });
		};

	server.onFilesChanged = [this](const uint32_t files)
		{
			if (!this)
			{
				return;
			}
			wxString str = wxString::Format("%u", files);
			this->CallAfter([this, str]() {lbl_value_Files->SetLabel(str); });
		};

	server.onTempFilesChanged = [this](const uint32_t temp_files)
		{
			if (!this)
			{
				return;
			}
			wxString str = wxString::Format("%u", temp_files);
			this->CallAfter([this, str]() {lbl_value_TempFiles->SetLabel(str); });
		};
}

// timers
void ServerGUI::StartTimers()
{
	nextCleanOldTime = wxDateTime::Now() + wxTimeSpan::Seconds(period_seconds_cleanOldFiles);
	nextCleanTempTime = wxDateTime::Now() + wxTimeSpan::Seconds(period_seconds_cleanTempFiles);

	timer_checkConnections.Start(period_ms_checkConnections);
	timer_cleanOldFiles.Start(period_seconds_cleanOldFiles * 1000);
	timer_cleanTempFiles.Start(period_seconds_cleanTempFiles * 1000);
	timer_updateLabels.Start(1000);
}
void ServerGUI::StopTimers()
{
	timer_checkConnections.Stop();
	timer_cleanOldFiles.Stop();
	timer_cleanTempFiles.Stop();
	timer_updateLabels.Stop();
}


// settings
void ServerGUI::LoadSettings()
{
	server.ApplySettings();

	settings.maxStorageGB = server.getStorageSpace();
	settings.maxUserFilesizeGB = server.getMaxUserFileSize();
	settings.storagePath = wxString::FromUTF8(server.getStoragePath());
	settings.enableCleaning = server.getCleaningEnable();


	// convert timers
	// seconds -> dd + hh + mm + ss

	// max time file existing
	uint32_t fileTimeExisting = server.getMaxTimeFileExisting();
	settings.max_time_fileExisting.d = static_cast<int>(fileTimeExisting / 86400);
	fileTimeExisting %= 86400;
	settings.max_time_fileExisting.h = static_cast<int>(fileTimeExisting / 3600);
	fileTimeExisting %= 3600;
	settings.max_time_fileExisting.m = static_cast<int>(fileTimeExisting / 60);
	settings.max_time_fileExisting.s = static_cast<int>(fileTimeExisting % 60);
	
	// max time temp file existing
	uint32_t tempFileTimeExisting = server.getMaxTimeTempFileExisting();
	settings.max_time_tempFileExisting.d = static_cast<int>(tempFileTimeExisting / 86400);
	tempFileTimeExisting %= 86400;
	settings.max_time_tempFileExisting.h = static_cast<int>(tempFileTimeExisting / 3600);
	tempFileTimeExisting %= 3600;
	settings.max_time_tempFileExisting.m = static_cast<int>(tempFileTimeExisting / 60);
	settings.max_time_tempFileExisting.s = static_cast<int>(tempFileTimeExisting % 60);

	// timer cleaning files
	uint32_t timerCleaningFiles = period_seconds_cleanOldFiles;
	settings.timer_CleaningFiles.d = static_cast<int>(timerCleaningFiles / 86400);
	timerCleaningFiles %= 86400;
	settings.timer_CleaningFiles.h = static_cast<int>(timerCleaningFiles / 3600);
	timerCleaningFiles %= 3600;
	settings.timer_CleaningFiles.m = static_cast<int>(timerCleaningFiles / 60);
	settings.timer_CleaningFiles.s = static_cast<int>(timerCleaningFiles % 60);

	// timer cleaning temp files
	uint32_t timerCleaningTempFiles = period_seconds_cleanTempFiles;
	settings.timer_CleaningTempFiles.d = static_cast<int>(timerCleaningTempFiles / 86400);
	timerCleaningTempFiles %= 86400;
	settings.timer_CleaningTempFiles.h = static_cast<int>(timerCleaningTempFiles / 3600);
	timerCleaningTempFiles %= 3600;
	settings.timer_CleaningTempFiles.m = static_cast<int>(timerCleaningTempFiles / 60);
	settings.timer_CleaningTempFiles.s = static_cast<int>(timerCleaningTempFiles % 60);

}

void ServerGUI::SaveSettings()
{
	server.setStorageSpace(settings.maxStorageGB);
	server.setMaxUserFileSize(settings.maxUserFilesizeGB);
	server.setCleaningEnable(settings.enableCleaning);

	if (!settings.enableCleaning)
	{
		lbl_value_NextCleanFiles->SetLabel("...");
		lbl_value_NextCleanTemp->SetLabel("...");
	}

	std::string path_str = settings.storagePath.ToStdString();
	if (path_str != server.getStoragePath())
	{
		server.setStoragePath(path_str);
	}

	// convert timers
	// dd + hh + mm + ss -> seconds
	
	server.setMaxTimeFileExisting(settings.max_time_fileExisting.s + 
								  settings.max_time_fileExisting.m * 60 + 
								  settings.max_time_fileExisting.h * 60 * 60 + 
								  settings.max_time_fileExisting.d * 60 * 60 * 24);

	server.setMaxTimeTempFileExisting(settings.max_time_tempFileExisting.s +
		settings.max_time_tempFileExisting.m * 60 +
		settings.max_time_tempFileExisting.h * 60 * 60 +
		settings.max_time_tempFileExisting.d * 60 * 60 * 24);

	period_seconds_cleanOldFiles = settings.timer_CleaningFiles.s +
								   settings.timer_CleaningFiles.m * 60 +
								   settings.timer_CleaningFiles.h * 60 * 60 +
								   settings.timer_CleaningFiles.d * 60 * 60 * 24;

	period_seconds_cleanTempFiles = settings.timer_CleaningTempFiles.s +
		settings.timer_CleaningTempFiles.m * 60 +
		settings.timer_CleaningTempFiles.h * 60 * 60 +
		settings.timer_CleaningTempFiles.d * 60 * 60 * 24;

	server.ApplySettings();
}

