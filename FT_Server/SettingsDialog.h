#pragma once

#include <wx/wx.h>
#include <wx/spinctrl.h>
#include <wx/slider.h>
#include <wx/filepicker.h>

#include "FolderManager.h"


struct TimeValues
{
    int d = 0;
    int h = 0;
    int m = 0;
    int s = 0;
};

struct ServerSettings
{
    int maxStorageGB;
    int maxUserFilesizeGB;
    wxString storagePath;
    bool enableCleaning;
    TimeValues max_time_fileExisting;
    TimeValues max_time_tempFileExisting;
    TimeValues timer_CleaningFiles;
    TimeValues timer_CleaningTempFiles;
};


class SettingsDialog : public wxDialog
{
public:
    SettingsDialog(wxWindow* parent, const ServerSettings& current);

    void GetSettings(ServerSettings& settings);

private:

    bool isStorageChanged = false;

    wxDirPickerCtrl* m_storagePath;
    wxSlider* m_storageSize;
    wxSlider* m_userFilesize;
    wxStaticText* m_storageSizeLabel;
    wxStaticText* m_userFilesizeLabel;

    uint64_t freeDiskSpace;
    int freeDiskSpaceGB;
    void setFreeDiskSpace();

    wxCheckBox* m_enableCleaning;

    // timers
    struct TimerControls
    {
        wxSpinCtrl* d;
        wxSpinCtrl* h;
        wxSpinCtrl* m;
        wxSpinCtrl* s;
    };
    TimerControls m_timers[4];

    
    void SetTimersEnabled(bool enable);
    void OnCleaningCheck(wxCommandEvent& evt);
    void OnSliderStorageChanged(wxCommandEvent& evt);
    void OnSliderUserFilesizeChanged(wxCommandEvent& evt);
    void OnStoragePathChanged(wxFileDirPickerEvent& event);

};



