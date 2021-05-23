#include "example.h"

#include <wxcode/awx/led.h>

#include <wx/sizer.h>

MainWindow::MainWindow(wxWindow* parent, wxWindowID id, const wxString& title,
                       const wxPoint& pos, const wxSize& size, long style)
    : wxFrame(parent, id, title, pos, size, style)
    , m_ledR(new awxLed(this))
    , m_ledG(new awxLed(this))
    , m_ledY(new awxLed(this))
{
    wxBoxSizer* sizer = new wxBoxSizer(wxVERTICAL);
    sizer->Add(m_ledR, 1, wxALIGN_CENTER | wxEXPAND | wxALL, 5);
    sizer->Add(m_ledG, 1, wxALIGN_CENTER | wxEXPAND | wxALL, 5);
    sizer->Add(m_ledY, 1, wxALIGN_CENTER | wxEXPAND | wxALL, 5);
    this->SetSizer(sizer);

    m_ledR->SetState(awxLED_BLINK);
    m_ledG->SetState(awxLED_BLINK);
    m_ledY->SetState(awxLED_BLINK);
    m_ledG->SetColour(awxLED_GREEN);
    m_ledY->SetColour(awxLED_YELLOW);

    this->SetMinSize(wxSize(150, 100));
    this->Layout();
    this->Center(wxCENTER_ON_SCREEN | wxBOTH);
}
