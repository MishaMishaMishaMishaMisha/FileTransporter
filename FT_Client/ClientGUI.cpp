#include "ClientGUI.h"
#include <fstream>


wxBEGIN_EVENT_TABLE(ClientGUI, wxFrame)
EVT_BUTTON(ID_BTN_CONNECT, ClientGUI::OnConnect)
EVT_BUTTON(ID_BTN_SELECT_PATH, ClientGUI::OnSelectDownloadPath)
EVT_BUTTON(ID_BTN_DOWNLOAD, ClientGUI::OnDownloadFile)
EVT_BUTTON(ID_BTN_UPLOAD, ClientGUI::OnUploadFile)
EVT_BUTTON(ID_BTN_CONTINUE, ClientGUI::OnContinueUpload)
EVT_BUTTON(ID_BTN_CANCEL, ClientGUI::OnCancel)
wxEND_EVENT_TABLE()



ClientGUI::ClientGUI(const wxString& title)
	: wxFrame(nullptr, wxID_ANY, title, wxDefaultPosition, wxSize(600, 400))
{
	// interface
	panel = new wxPanel(this);

	// buttons
	btnConnect = new wxButton(panel, ID_BTN_CONNECT, "Reconnect to server");
	btnSelectPath = new wxButton(panel, ID_BTN_SELECT_PATH, "Select download path");
	btnDownload = new wxButton(panel, ID_BTN_DOWNLOAD, "Download file");
	btnUpload = new wxButton(panel, ID_BTN_UPLOAD, "Upload file");
	btnContinue = new wxButton(panel, ID_BTN_CONTINUE, "Continue uploading file");
	btnCancel = new wxButton(panel, ID_BTN_CANCEL, "Cancel");

	// buttons width
	int maxWidth = std::max({
		btnConnect->GetBestSize().GetWidth(),
		btnSelectPath->GetBestSize().GetWidth(),
		btnDownload->GetBestSize().GetWidth(),
		btnUpload->GetBestSize().GetWidth(),
		btnContinue->GetBestSize().GetWidth(),
		btnCancel->GetBestSize().GetWidth()
		});

	wxSize buttonSize(maxWidth, btnConnect->GetBestSize().GetHeight());
	btnConnect->SetMinSize(buttonSize);
	btnSelectPath->SetMinSize(buttonSize);
	btnDownload->SetMinSize(buttonSize);
	btnUpload->SetMinSize(buttonSize);
	btnContinue->SetMinSize(buttonSize);
	btnCancel->SetMinSize(buttonSize);


	// console for information
	consoleOutput = new wxTextCtrl(panel, wxID_ANY, "", wxDefaultPosition, wxSize(-1, 100),
		wxTE_MULTILINE | wxTE_READONLY);


	// lable over progress bar
	label = new wxStaticText(panel, wxID_ANY, "Progress", wxDefaultPosition);
	// progress bar
	gauge = new wxGauge(panel, wxID_ANY, 100, wxDefaultPosition, wxSize(350, 25));
	gauge->SetValue(0);


	// layout
	wxBoxSizer* vbox = new wxBoxSizer(wxVERTICAL);

	// first row for buttons
	wxBoxSizer* hbox1 = new wxBoxSizer(wxHORIZONTAL);
	hbox1->Add(btnConnect, 1, wxALL, 5);
	hbox1->Add(btnSelectPath, 1, wxALL, 5);
	hbox1->Add(btnDownload, 1, wxALL, 5);

	// second row for buttons
	wxBoxSizer* hbox2 = new wxBoxSizer(wxHORIZONTAL);
	hbox2->Add(btnUpload, 1, wxALL, 5);
	hbox2->Add(btnContinue, 1, wxALL, 5);
	hbox2->Add(btnCancel, 1, wxALL, 5);


	vbox->Add(hbox1, 0, wxEXPAND | wxLEFT | wxRIGHT | wxTOP, 5);
	vbox->Add(hbox2, 0, wxEXPAND | wxLEFT | wxRIGHT, 5);

	vbox->Add(consoleOutput, 1, wxEXPAND | wxALL, 5);


	wxBoxSizer* progressBlock = new wxBoxSizer(wxVERTICAL);
	wxBoxSizer* labelBox = new wxBoxSizer(wxHORIZONTAL);
	labelBox->Add(label, 0, wxALIGN_LEFT | wxBOTTOM, 3);
	labelBox->AddStretchSpacer(1);
	progressBlock->Add(labelBox, 0, wxEXPAND | wxLEFT | wxRIGHT, 5);
	progressBlock->Add(gauge, 0, wxEXPAND | wxLEFT | wxRIGHT, 5);
	vbox->Add(progressBlock, 0, wxEXPAND | wxALL, 5);

	panel->SetSizer(vbox);



	// set callback function for messages
	client.onMessage = [this](const std::string& msg) 
		{
			wxString wxs(msg.c_str(), wxConvUTF8);
			this->CallAfter(&ClientGUI::PrintToConsole, wxs); 
		};

	// callback function for finish work
	client.onFinished = [this]() 
		{ 
			EnableButtons(true);
			btnCancel->Enable(false);
			label->SetLabel("Done");
			gauge->SetValue(0);
		};

	// callback function for progress
	client.onProgress = [this](const uint64_t currentValue, const uint64_t maxValue)
		{
			if (!client.IsConnected())
			{
				return;
			}

			// calculate progress
			if (maxValue == 0) return;

			progress = static_cast<int>((static_cast<double>(currentValue) / maxValue) * 100);

			std::string text = "Progress (" + std::to_string(currentValue) + "/" + std::to_string(maxValue) + " MB)";
			label->SetLabel(text);
			gauge->SetValue(progress);
		};


	ft::filehash::onCalculatingProgress = [this](const uint64_t value)
		{
			label->SetLabel("Check hash file...");
			gauge->SetValue(value);
		};


	btnCancel->Enable(false);

}

ClientGUI::~ClientGUI()
{
	StopClientsWork();
}


// events

void ClientGUI::OnConnect(wxCommandEvent& event)
{
	AddTask([this]
		{
			if (!running)
			{
				return;
			}

			if (!client.IsConnected())
			{
				PrintToConsole("Reconnecting to server...");
				client.Connect(server_ip, server_port,
					[this](bool success)
					{

						// set callback function for connection lost
						// (set it every reconnecting to server)
						client.setOnConnectionLostFunction([this]()
							{
								EnableButtons(false);
								btnConnect->Enable(true);
								btnCancel->Enable(false);
								label->SetLabel("Progress");
								gauge->SetValue(0);
								this->CallAfter(&ClientGUI::PrintToConsole, "Disconnect from server");
							});

						if (success)
						{
							running = true;
							EnableButtons(true);
							PrintToConsole("Connected to server!");
						}
						else
						{
							PrintToConsole("Cant connect to server");
						}
					});
			}

		});
	
}

void ClientGUI::OnSelectDownloadPath(wxCommandEvent& event)
{
	wxDirDialog dlg(this, "Select download folder", "", wxDD_DEFAULT_STYLE | wxDD_DIR_MUST_EXIST);
	if (dlg.ShowModal() == wxID_OK)
	{
		downloadPath = dlg.GetPath();

		std::string path_str = downloadPath.ToStdString() + "\\";
		client.SetDownloadsPath(path_str);
	}
}

void ClientGUI::OnDownloadFile(wxCommandEvent& event)
{
	wxTextEntryDialog dlg(this, "Enter id of your file: ", "ID INPUT");
	if (dlg.ShowModal() == wxID_OK)
	{
		wxString inputText = dlg.GetValue();
		fileID = inputText.ToStdString();


		AddTask([this]
			{
				if (client.IsConnected())
				{
					if (!fileID.empty())
					{
						EnableButtons(false);
						btnCancel->Enable(true);
						client.DownloadFile(fileID);
					}
					else
					{
						PrintToConsole("Please, enter a non-empty ID");
					}

				}
				else
				{
					PrintToConsole("Please, reconnect to server");
				}

			});
	}

}

void ClientGUI::OnUploadFile(wxCommandEvent& event)
{
	wxFileDialog dlg(this, "Select file to upload", "", "", "All files (*.*)|*.*", wxFD_OPEN | wxFD_FILE_MUST_EXIST);
	
	if (dlg.ShowModal() == wxID_OK)
	{
		uploadFilePath = dlg.GetPath();

		AddTask([this]
			{
				if (client.IsConnected())
				{
					EnableButtons(false);
					btnCancel->Enable(true);
					std::string path_str = uploadFilePath.ToStdString();
					
					if (!client.UploadFile(path_str))
					{
						EnableButtons(true);
						btnCancel->Enable(false);
					}

				}
				else
				{
					PrintToConsole("Please, reconnect to server");
					EnableButtons(true);
					btnCancel->Enable(false);
				}
				
			});

	}
}

void ClientGUI::OnContinueUpload(wxCommandEvent& event)
{
	wxFileDialog dlg(this, "Select file to continue upload", "", "", "All files (*.*)|*.*", wxFD_OPEN | wxFD_FILE_MUST_EXIST);
	
	if (dlg.ShowModal() == wxID_OK)
	{
		uploadFilePath = dlg.GetPath();
		
		std::string path_str = uploadFilePath.ToStdString();
		if (!client.CheckFile(path_str))
		{
			PrintToConsole("Error while reading file. Try again");
			return;
		}
		

		// id input dialog
		wxTextEntryDialog dlg(this, "Enter id of your file: ", "ID INPUT");
		if (dlg.ShowModal() == wxID_OK)
		{
			wxString inputText = dlg.GetValue();
			fileID = inputText.ToStdString();

			AddTask([this]
				{
					if (client.IsConnected())
					{
						if (!fileID.empty())
						{
							EnableButtons(false);
							btnCancel->Enable(true);
							client.ContinueUploadFile(fileID);
						}
						else
						{
							PrintToConsole("Please, enter a non-empty ID");
						}
					}
					else
					{
						PrintToConsole("Please, reconnect to server");
					}

				});

		}

	}

}

void ClientGUI::OnCancel(wxCommandEvent& event)
{
	client.CancelTranseringFile();
	btnCancel->Enable(false);
}


void ClientGUI::PrintToConsole(const wxString& message)
{
	consoleOutput->AppendText(message + "\n");
}



// client's work
void ClientGUI::LaunchClient()
{
	client.StartThreadHandleMessages();

	running = true;
	thr_clientWork = std::thread(&ClientGUI::HandleClientsTasks, this);

	AddTask([this]
		{
			if (!running) 
			{
				return;
			}


			PrintToConsole("Connecting to server...");

			client.Connect(server_ip, server_port,
				[this](bool success) 
				{

					// set call back function for connection lost 
					client.setOnConnectionLostFunction([this]()
						{
							EnableButtons(false);
							btnConnect->Enable(true);
							btnCancel->Enable(false);
							label->SetLabel("Progress");
							gauge->SetValue(0);
							this->CallAfter(&ClientGUI::PrintToConsole, "Disconnect from server");
						});

					if (success)
					{
						PrintToConsole("Connected to server!");
					}
					else
					{
						PrintToConsole("Cant connect to server");

						EnableButtons(false);
						btnConnect->Enable(true);
						btnCancel->Enable(false);
					}
				});

		});

}

void ClientGUI::StopClientsWork()
{
	ft::filehash::IsCancelCalculatingHash.store(true);

	client.StopThreadHandleMessages();

	running = false;
	{
		std::lock_guard<std::mutex> lock(mtx);
		std::queue<std::function<void()>> empty;
		tasks.swap(empty);
	}
	cv.notify_one();
	client.Disconnect();
	if (thr_clientWork.joinable())
	{
		thr_clientWork.join();
	}

}

void ClientGUI::AddTask(std::function<void()> task) 
{
	{
		std::lock_guard<std::mutex> lock(mtx);
		tasks.push(std::move(task));
	}
	cv.notify_one();
}

void ClientGUI::HandleClientsTasks()
{
	while (running) 
	{
		std::function<void()> task;

		{
			std::unique_lock<std::mutex> lock(mtx);
			cv.wait(lock, [this] { return !tasks.empty() || !running; });

			if (!running && tasks.empty())
			{
				break;
			}

			task = std::move(tasks.front());
			tasks.pop();
		}

		task(); // execute task
	}
}

void ClientGUI::EnableButtons(bool enable)
{
	btnConnect->Enable(enable);
	btnSelectPath->Enable(enable);
	btnDownload->Enable(enable);
	btnUpload->Enable(enable);
	btnContinue->Enable(enable);
}