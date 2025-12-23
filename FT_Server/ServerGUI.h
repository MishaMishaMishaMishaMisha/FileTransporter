#pragma once


#include <wx/wx.h>
#include <wx/filedlg.h>
#include <wx/dirdlg.h>
#include <wx/textctrl.h>
#include <wx/gauge.h>
#include <wx/event.h>
#include <wx/utils.h>
#include <wx/datetime.h>

#include "CustomServer.h"
#include "SettingsDialog.h"

#include <iostream>
#include <thread>
#include <queue>
#include <mutex>
#include <condition_variable>
#include <functional>
#include <atomic>




class ServerGUI : public wxFrame
{
public:
	ServerGUI(const wxString& title);
	~ServerGUI();

	void LaunchServer();
	void StopServer();

private:

	// set PORT for your server
	uint16_t port = 60000;

	CustomServer server;

	std::thread thr_serverWork;
	std::atomic<bool> running{ false };

	ServerSettings settings;
	void LoadSettings();
	void SaveSettings();

	wxString storagePath;

	void SetCallbackFunctions();

	// timers
	int period_ms_checkConnections = 5000; // check connections every 5 seconds
	int period_seconds_cleanOldFiles = 12 * 60 * 60; // by default, delete old files every 12 hours
	int period_seconds_cleanTempFiles = 6 * 60 * 60; // by default, delete old tempFiles every 6 hours

	wxTimer timer_checkConnections;
	wxTimer timer_cleanOldFiles;
	wxTimer timer_cleanTempFiles;

	// for print timers
	wxTimer timer_updateLabels;
	wxDateTime nextCleanOldTime;
	wxDateTime nextCleanTempTime;

	void OnTimer_CheckConnections(wxTimerEvent& event);
	void OnTimer_CleanOldFiles(wxTimerEvent& event);
	void OnTimer_CleanTempFiles(wxTimerEvent& event);
	void OnUpdateLabels(wxTimerEvent&);

	void StartTimers();
	void StopTimers();


	void PrintToConsole(const wxString& message);


	// interface
	wxTextCtrl* consoleOutput;

	wxButton* btnSettings;
	wxButton* btnClearStorage;
	wxButton* btnStartServer;
	wxButton* btnStopServer;

	wxStaticText* lbl_value_ServerStatus;
	wxStaticText* lbl_value_FreeSpace;
	wxStaticText* lbl_value_ReservedSpace;
	wxStaticText* lbl_value_Files;
	wxStaticText* lbl_value_TempFiles;
	wxStaticText* lbl_value_Users;
	wxStaticText* lbl_value_NextCleanFiles;
	wxStaticText* lbl_value_NextCleanTemp;


	// button events
	void OnSettings(wxCommandEvent& event);
	void OnClearStorage(wxCommandEvent& event);
	void OnStartServer(wxCommandEvent& event);
	void OnStopServer(wxCommandEvent& event);


	wxDECLARE_EVENT_TABLE();
};

enum
{
	ID_BTN_SETTINGS = wxID_HIGHEST + 5,
	ID_BTN_CLEAR_STORAGE,
	ID_BTN_START,
	ID_BTN_STOP
};






