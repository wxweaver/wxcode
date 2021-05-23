#pragma once

#include <wx/frame.h>
#include <wx/timer.h>

class wxLedBarGraph;

class MainWindow : public wxFrame {
public:
    MainWindow(wxWindow* parent = nullptr, wxWindowID id = wxID_ANY,
               const wxString& title = _("wxLedBarGraph Test"),
               const wxPoint& pos = wxDefaultPosition,
               const wxSize& size = wxSize(500, 300),
               long style = wxSYSTEM_MENU | wxCLOSE_BOX | wxCAPTION);
    ~MainWindow();

private:
    void onTimer(wxTimerEvent&);

    wxLedBarGraph* m_ledbgL;
    wxLedBarGraph* m_ledbgR;
    wxTimer m_timer;
};
