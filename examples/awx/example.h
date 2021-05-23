#pragma once

#include <wx/frame.h>

class awxLed;

class MainWindow : public wxFrame {
public:
    MainWindow(wxWindow* parent = nullptr, wxWindowID id = wxID_ANY,
               const wxString& title = _("awxLed Test"),
               const wxPoint& pos = wxDefaultPosition,
               const wxSize& size = wxSize(150, 100),
               long style = wxDEFAULT_FRAME_STYLE);

private:
    awxLed* m_ledR;
    awxLed* m_ledG;
    awxLed* m_ledY;
};
