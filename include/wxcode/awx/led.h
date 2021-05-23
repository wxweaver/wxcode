/*
    Name:      led.cpp
    Author:    Joachim Buermann
    Copyright: (c) 2001 Joachim Buermann
*/
#pragma once

#ifndef WX_PRECOMP
#include "wx/wx.h"
#endif

enum awxLedState {
    awxLED_OFF = 0,
    awxLED_ON,
    awxLED_BLINK
};

enum awxLedColour {
    awxLED_LUCID = 0,
    awxLED_RED,
    awxLED_GREEN,
    awxLED_YELLOW
};

class BlinkTimer;

class awxLed : public wxWindow {
public:
    awxLed(wxWindow* parent,
           wxWindowID id = wxID_ANY,
           const wxPoint& pos = wxPoint(0, 0),
           const wxSize& size = wxSize(15, 15),
           long style = 0);
    ~awxLed();

    void Blink();
    void OnErase(wxEraseEvent&) { Redraw(); };
    void OnPaint(wxPaintEvent&)
    {
        wxPaintDC dc(this);
        dc.DrawBitmap(*m_bitmap, 0, 0, false);
    };

    void OnSizeEvent(wxSizeEvent& event)
    {
        wxSize size = event.GetSize();
        m_x = (size.GetX() - m_icons[0]->GetWidth()) >> 1;
        m_y = (size.GetY() - m_icons[0]->GetHeight()) >> 1;
        if (m_x < 0)
            m_x = 0;
        if (m_y < 0)
            m_y = 0;
        DrawOnBitmap();
    };

    void Redraw()
    {
        wxClientDC dc(this);
        DrawOnBitmap();
        dc.DrawBitmap(*m_bitmap, 0, 0, false);
    };

    void SetTimerInterval(unsigned int timerInterval);
    unsigned int GetTimerInterval();
    void SetColour(awxLedColour colour);
    void SetState(awxLedState state);
    void SetOn(awxLedColour colour, awxLedState state = awxLED_ON);
    void SetOff(awxLedColour colour, awxLedState state = awxLED_ON);
    void TurnOn(bool on = true);
    void TurnOff();
    void Toggle();
    bool IsOn();

protected:
    void DrawOnBitmap();

    wxBitmap* m_bitmap; // bitmap for double buffering
    wxIcon* m_icons[2];
    awxLedState m_state;
    BlinkTimer* m_timer;
    int m_blink;
    int m_x;
    int m_y;
    unsigned int m_timerInterval;
    bool m_on;
    awxLedState m_onState;
    awxLedState m_offState;
    awxLedColour m_onColour;
    awxLedColour m_offColour;
};

class BlinkTimer : public wxTimer {
public:
    BlinkTimer(awxLed* led)
        : wxTimer()
        , m_led(led) {};

    void Notify() { m_led->Blink(); };

protected:
    awxLed* m_led;
};
