#include "SettingsDialog.h"


SettingsDialog::SettingsDialog(wxWindow* parent, const ServerSettings& current)
    : wxDialog(parent, wxID_ANY, "Server settings",
        wxDefaultPosition, wxSize(520, 520),
        wxDEFAULT_DIALOG_STYLE | wxRESIZE_BORDER)
{



    wxBoxSizer* mainSizer = new wxBoxSizer(wxVERTICAL);

    // ===== Storage path =====
    mainSizer->Add(new wxStaticText(this, wxID_ANY, "Storage path"), 0, wxALL, 5);

    m_storagePath = new wxDirPickerCtrl(this, wxID_ANY, current.storagePath, "Select storage directory");

    m_storagePath->GetTextCtrl()->SetEditable(false);

    mainSizer->Add(m_storagePath, 0, wxEXPAND | wxLEFT | wxRIGHT, 5);


    setFreeDiskSpace();

    // ===== sliders =====
    mainSizer->Add(new wxStaticText(this, wxID_ANY, "Storage space"), 0, wxALL, 5);

    // ---------- first slider ----------
    wxBoxSizer* sizeBox1 = new wxBoxSizer(wxHORIZONTAL);

    m_storageSize = new wxSlider(this, wxID_ANY,
        current.maxStorageGB, 1, freeDiskSpaceGB,
        wxDefaultPosition, wxDefaultSize,
        wxSL_HORIZONTAL);

    m_storageSizeLabel = new wxStaticText(this, wxID_ANY, wxString::Format("%d GB", current.maxStorageGB));

    sizeBox1->Add(m_storageSize, 1, wxRIGHT, 10);
    sizeBox1->Add(m_storageSizeLabel, 0, wxALIGN_CENTER_VERTICAL);

    mainSizer->Add(sizeBox1, 0, wxEXPAND | wxLEFT | wxRIGHT, 5);

    // ---------- second slider ----------
    mainSizer->Add(new wxStaticText(this, wxID_ANY, "Max user's file size"), 0, wxALL, 5);

    wxBoxSizer* sizeBox2 = new wxBoxSizer(wxHORIZONTAL);

    m_userFilesize = new wxSlider(this, wxID_ANY,
        current.maxUserFilesizeGB, 0, current.maxStorageGB,
        wxDefaultPosition, wxDefaultSize,
        wxSL_HORIZONTAL);

    m_userFilesizeLabel = new wxStaticText(this, wxID_ANY, wxString::Format("%d GB", current.maxUserFilesizeGB));

    sizeBox2->Add(m_userFilesize, 1, wxRIGHT, 10);
    sizeBox2->Add(m_userFilesizeLabel, 0, wxALIGN_CENTER_VERTICAL);

    mainSizer->Add(sizeBox2, 0, wxEXPAND | wxLEFT | wxRIGHT, 5);

    // ---------- events ----------
    m_storageSize->Bind(wxEVT_SLIDER, &SettingsDialog::OnSliderStorageChanged, this);

    m_userFilesize->Bind(wxEVT_SLIDER, &SettingsDialog::OnSliderUserFilesizeChanged, this);

    m_storagePath->Bind(wxEVT_DIRPICKER_CHANGED, &SettingsDialog::OnStoragePathChanged, this);



    // ===== Enable cleaning =====
    m_enableCleaning = new wxCheckBox( this, wxID_ANY, "Enable cleaning");

    m_enableCleaning->SetValue(current.enableCleaning);

    mainSizer->Add(m_enableCleaning, 0, wxALL, 10);

    m_enableCleaning->Bind(wxEVT_CHECKBOX, &SettingsDialog::OnCleaningCheck, this);

    // ===== Timers =====
    const char* labels[4] =
    {
        "Max files lifetime",
        "Max temp files lifetime",
        "Period of cleaning up old storage files",
        "Period of cleaning up old temp storage files"
    };

    const TimeValues* values[4] =
    {
        &current.max_time_fileExisting,
        &current.max_time_tempFileExisting,
        &current.timer_CleaningFiles,
        &current.timer_CleaningTempFiles
    };

    for (int i = 0; i < 4; ++i)
    {
        wxStaticBoxSizer* box = new wxStaticBoxSizer(wxHORIZONTAL, this, labels[i]);

        m_timers[i].d = new wxSpinCtrl(this, wxID_ANY, "",
            wxDefaultPosition, wxSize(60, -1),
            wxSP_ARROW_KEYS, 0, 30, values[i]->d);

        m_timers[i].h = new wxSpinCtrl(this, wxID_ANY, "",
            wxDefaultPosition, wxSize(60, -1),
            wxSP_ARROW_KEYS, 0, 23, values[i]->h);

        m_timers[i].m = new wxSpinCtrl(this, wxID_ANY, "",
            wxDefaultPosition, wxSize(60, -1),
            wxSP_ARROW_KEYS, 0, 59, values[i]->m);

        m_timers[i].s = new wxSpinCtrl(this, wxID_ANY, "",
            wxDefaultPosition, wxSize(60, -1),
            wxSP_ARROW_KEYS, 0, 59, values[i]->s);

        box->Add(new wxStaticText(this, wxID_ANY, "D"), 0, wxALIGN_CENTER | wxRIGHT, 3);
        box->Add(m_timers[i].d, 0, wxRIGHT, 10);
        box->Add(new wxStaticText(this, wxID_ANY, "H"), 0, wxALIGN_CENTER | wxRIGHT, 3);
        box->Add(m_timers[i].h, 0, wxRIGHT, 10);
        box->Add(new wxStaticText(this, wxID_ANY, "M"), 0, wxALIGN_CENTER | wxRIGHT, 3);
        box->Add(m_timers[i].m, 0, wxRIGHT, 10);
        box->Add(new wxStaticText(this, wxID_ANY, "S"), 0, wxALIGN_CENTER | wxRIGHT, 3);
        box->Add(m_timers[i].s, 0);

        mainSizer->Add(box, 0, wxEXPAND | wxLEFT | wxRIGHT | wxBOTTOM, 5);
    }

    // initial state
    SetTimersEnabled(current.enableCleaning);

    // ===== Buttons =====
    wxStdDialogButtonSizer* btnSizer = new wxStdDialogButtonSizer();
    btnSizer->AddButton(new wxButton(this, wxID_OK));
    btnSizer->AddButton(new wxButton(this, wxID_CANCEL));
    btnSizer->Realize();

    mainSizer->Add(btnSizer, 0, wxALIGN_RIGHT | wxALL, 10);

    SetSizerAndFit(mainSizer);
}



void SettingsDialog::SetTimersEnabled(bool enable)
{
    for (auto& t : m_timers)
    {
        t.d->Enable(enable);
        t.h->Enable(enable);
        t.m->Enable(enable);
        t.s->Enable(enable);
    }
}

void SettingsDialog::OnCleaningCheck(wxCommandEvent&)
{
    SetTimersEnabled(m_enableCleaning->GetValue());
}

void SettingsDialog::OnSliderStorageChanged(wxCommandEvent& evt)
{
    m_storageSizeLabel->SetLabel(wxString::Format("%d GB", m_storageSize->GetValue()));

    m_userFilesize->SetMax(m_storageSize->GetValue());
    m_userFilesize->SetValue(1);
    m_userFilesizeLabel->SetLabel(wxString::Format("%d GB", 1));
}
void SettingsDialog::OnSliderUserFilesizeChanged(wxCommandEvent& evt)
{
    m_userFilesizeLabel->SetLabel(wxString::Format("%d GB", m_userFilesize->GetValue()));
}

void SettingsDialog::OnStoragePathChanged(wxFileDirPickerEvent& event)
{
    setFreeDiskSpace();

    m_storageSize->SetValue(1);
    m_storageSize->SetMax(freeDiskSpaceGB);
    m_storageSizeLabel->SetLabel(wxString::Format("%d GB", 1));

    m_userFilesize->SetValue(1);
    m_userFilesize->SetMax(1);
    m_userFilesizeLabel->SetLabel(wxString::Format("%d GB", 1));
}


void SettingsDialog::GetSettings(ServerSettings& settings)
{
    settings.storagePath = m_storagePath->GetPath();
    settings.maxUserFilesizeGB = m_userFilesize->GetValue();
    settings.maxStorageGB = m_storageSize->GetValue();
    settings.enableCleaning = m_enableCleaning->GetValue();

    settings.max_time_fileExisting =
    {
        m_timers[0].d->GetValue(),
        m_timers[0].h->GetValue(),
        m_timers[0].m->GetValue(),
        m_timers[0].s->GetValue()
    };

    settings.max_time_tempFileExisting =
    {
        m_timers[1].d->GetValue(),
        m_timers[1].h->GetValue(),
        m_timers[1].m->GetValue(),
        m_timers[1].s->GetValue()
    };

    settings.timer_CleaningFiles =
    {
        m_timers[2].d->GetValue(),
        m_timers[2].h->GetValue(),
        m_timers[2].m->GetValue(),
        m_timers[2].s->GetValue()
    };

    settings.timer_CleaningTempFiles =
    {
        m_timers[3].d->GetValue(),
        m_timers[3].h->GetValue(),
        m_timers[3].m->GetValue(),
        m_timers[3].s->GetValue()
    };

}


void SettingsDialog::setFreeDiskSpace()
{
    std::string currentPath = m_storagePath->GetPath().ToStdString();
    freeDiskSpace = ft::fm::GetFreeDiskSpace(currentPath);
    freeDiskSpaceGB = static_cast<int>(freeDiskSpace >> 30);
}