#pragma once

#include <wx/wx.h>
#include <wx/filedlg.h>
#include <wx/dirdlg.h>
#include <wx/textctrl.h>
#include <wx/gauge.h>
#include <wx/event.h>
#include <wx/utils.h>

#include "CustomClient.h"

#include <iostream>
#include <thread>
#include <queue>
#include <mutex>
#include <condition_variable>
#include <functional>
#include <atomic>




class ClientGUI : public wxFrame
{
public:
	ClientGUI(const wxString& title);
	~ClientGUI();

	void LaunchClient();

private:

	// set IP and PORT of your server
	std::string server_ip = "127.0.0.1";
	uint16_t server_port = 60000;

	// client
	CustomClient client;
	std::thread thr_clientWork;
	std::atomic<bool> running{ false };
	std::queue<std::function<void()>> tasks;
	std::mutex mtx;
	std::condition_variable cv;
	void StopClientsWork();
	void HandleClientsTasks();
	void AddTask(std::function<void()> task);
	std::string fileID = "";

	// interface
	wxPanel* panel;
	wxButton* btnConnect;
	wxButton* btnSelectPath;
	wxButton* btnDownload;
	wxButton* btnUpload;
	wxButton* btnContinue;
	wxButton* btnCancel;
	wxTextCtrl* consoleOutput;


	wxString downloadPath;
	wxString uploadFilePath;

	// progress bar
	wxGauge* gauge;
	wxStaticText* label;
	int progress = 0;

	// button events
	void OnConnect(wxCommandEvent& event);
	void OnSelectDownloadPath(wxCommandEvent& event);
	void OnDownloadFile(wxCommandEvent& event);
	void OnUploadFile(wxCommandEvent& event);
	void OnContinueUpload(wxCommandEvent& event);
	void OnCancel(wxCommandEvent& event);


	void PrintToConsole(const wxString& message);

	void EnableButtons(bool enable);

	wxDECLARE_EVENT_TABLE();
};

enum
{
	ID_BTN_CONNECT = wxID_HIGHEST + 1,
	ID_BTN_SELECT_PATH,
	ID_BTN_DOWNLOAD,
	ID_BTN_UPLOAD,
	ID_BTN_CONTINUE,
	ID_BTN_CANCEL
};




