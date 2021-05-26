#include "example.h"

#include <wxcode/ledbargraph/ledbargraph.h>

#include <wx/sizer.h>

#include <random>

MainWindow::MainWindow(wxWindow* parent, wxWindowID id, const wxString& title,
                       const wxPoint& pos, const wxSize& size, long style)
    : wxFrame(parent, id, title, pos, size, style)
    , m_ledbgL(new wxLedBarGraph(this))
    , m_ledbgR(new wxLedBarGraph(this))
{
    wxBoxSizer* sizer = new wxBoxSizer(wxVERTICAL);
    sizer->Add(m_ledbgL, 1, wxALIGN_CENTER | wxEXPAND | wxALL, 5);
    sizer->Add(m_ledbgR, 1, wxALIGN_CENTER | wxEXPAND | wxALL, 5);
    this->SetSizer(sizer);

    m_timer.SetOwner(this, wxID_ANY);
    m_timer.Start(100);

    m_ledbgL->SetDrawingMode(wxLED_BG_SINGLE_SIDED_TOP_LEFT);
    m_ledbgR->SetDrawingMode(wxLED_BG_SINGLE_SIDED_TOP_LEFT);
    m_ledbgL->SetSizingMode(wxLED_BG_FIXED_BAR_COUNT);
    m_ledbgR->SetSizingMode(wxLED_BG_FIXED_BAR_COUNT);
    m_ledbgR->SetOrientation(wxHORIZONTAL);
    m_ledbgL->SetOrientation(wxHORIZONTAL);
    m_ledbgL->SetBarWidths(8);
    m_ledbgR->SetBarWidths(8);
    m_ledbgL->SetBarCount(28);
    m_ledbgR->SetBarCount(28);
    m_ledbgL->SetMinValue(-10);
    m_ledbgR->SetMinValue(-10);
    m_ledbgL->SetMaxValue(10);
    m_ledbgR->SetMaxValue(10);

    this->SetMaxSize(wxSize(400, 90));
    this->Layout();
    this->Center(wxCENTER_ON_SCREEN | wxBOTH);

    this->Bind(wxEVT_TIMER, &MainWindow::onTimer, this);
}

MainWindow::~MainWindow()
{
    if (m_timer.IsRunning())
        m_timer.Stop();
}

void MainWindow::onTimer(wxTimerEvent&)
{
    std::random_device rd;
    std::mt19937 gen(rd());
    std::uniform_real_distribution<> dist(-10, 10);

    double value = dist(gen);
    m_ledbgL->SetValue(value);
    m_ledbgR->SetValue(value);
}
