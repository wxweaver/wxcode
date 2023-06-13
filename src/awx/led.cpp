/*
    Name:      led.cpp
    Author:    Joachim Buermann
    Copyright: (c) 2001 Joachim Buermann
*/
#include <wxcode/awx/led.h>
#include "leds.xpm"

awxLed::awxLed(wxWindow* parent,
               wxWindowID id,
               const wxPoint& pos,
               const wxSize& size,
               long style)
    : wxWindow(parent, id, pos, size, wxNO_FULL_REPAINT_ON_RESIZE | style)
    , m_bitmap(new wxBitmap(15, 15))
    , m_state(awxLED_OFF)
    , m_timer(new BlinkTimer(this))
    , m_blink(0)
    , m_x(0)
    , m_y(0)
    , m_timerInterval(500)
    , m_on(false)
{
    m_icons[awxLED_OFF] = new wxIcon((const char**)led_off_xpm);
    m_icons[awxLED_ON] = nullptr;

    SetInitialSize(wxSize(15, 15));
    SetMinSize(wxSize(15, 15));
    SetColour(awxLED_RED);

    Bind(wxEVT_ERASE_BACKGROUND, &awxLed::OnErase, this);
    Bind(wxEVT_PAINT, &awxLed::OnPaint, this);
    Bind(wxEVT_SIZE, &awxLed::OnSizeEvent, this);
};

awxLed::~awxLed()
{
    if (m_timer) {
        m_timer->Stop();
        delete m_timer;
    }
    delete m_bitmap;
    delete m_icons[awxLED_OFF];
    delete m_icons[awxLED_ON];
};

void awxLed::Blink()
{
    m_blink ^= 1;
    Redraw();
};

void awxLed::DrawOnBitmap()
{
    wxSize s = GetClientSize();
    if ((m_bitmap->GetWidth() != s.GetWidth()) || (m_bitmap->GetHeight() != s.GetHeight())) {
        m_bitmap->Create(s.x, s.y);
    }
    wxMemoryDC dc;
    dc.SelectObject(*m_bitmap);

    wxBrush brush(this->GetBackgroundColour(), wxBRUSHSTYLE_SOLID);
    dc.SetBackground(brush);
    dc.Clear();

    if (m_state == awxLED_BLINK)
        dc.DrawIcon(*m_icons[m_blink], m_x, m_y);
    else
        dc.DrawIcon(*m_icons[m_state & 1], m_x, m_y);

    dc.SelectObject(wxNullBitmap);
};

void awxLed::SetColour(awxLedColour colour)
{
    if (m_icons[awxLED_ON])
        delete m_icons[awxLED_ON];

    switch (colour) {
    case awxLED_LUCID:
        m_icons[awxLED_ON] = new wxIcon((const char**)led_off_xpm);
        break;
    case awxLED_GREEN:
        m_icons[awxLED_ON] = new wxIcon((const char**)led_green_xpm);
        break;
    case awxLED_YELLOW:
        m_icons[awxLED_ON] = new wxIcon((const char**)led_yellow_xpm);
        break;
    default:
        m_icons[awxLED_ON] = new wxIcon((const char**)led_red_xpm);
    }
};

void awxLed::SetState(awxLedState state)
{
    m_state = state;
    if (m_timer->IsRunning())
        m_timer->Stop();

    if (m_state == awxLED_BLINK)
        m_timer->Start(m_timerInterval);

    Redraw();
};

void awxLed::SetOn(awxLedColour colour, awxLedState state)
{
    m_onColour = colour;
    m_onState = state;
}

void awxLed::SetOff(awxLedColour colour, awxLedState state)
{
    m_offColour = colour;
    m_offState = state;
}

void awxLed::TurnOn(bool on)
{
    m_on = on;
    if (on) {
        SetColour(m_onColour);
        SetState(m_onState);
    } else {
        SetColour(m_offColour);
        SetState(m_offState);
    }
}

void awxLed::TurnOff()
{
    m_on = false;
    SetColour(m_offColour);
    SetState(m_offState);
}

void awxLed::Toggle()
{
    TurnOn(!m_on);
}

bool awxLed::IsOn()
{
    return m_on;
}

void awxLed::SetTimerInterval(unsigned int timerInterval)
{
    m_timerInterval = timerInterval;
    SetState(m_state);
}

unsigned int awxLed::GetTimerInterval()
{
    return m_timerInterval;
}
