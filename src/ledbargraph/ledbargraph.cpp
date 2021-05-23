#include <wxcode/ledbargraph/ledbargraph.h>

wxIMPLEMENT_DYNAMIC_CLASS(wxLedBarGraph, wxControl);

bool wxLedBarGraph::Create(wxWindow* parent, wxWindowID id,
                           const wxPoint& pos, const wxSize& size, long style,
                           const wxValidator& validator)
{
    if (!wxControl::Create(parent, id, pos, size, style, validator))
        return false;

    SetBackgroundColour(wxSystemSettings::GetColour(wxSYS_COLOUR_WINDOW));

    Bind(wxEVT_PAINT, &wxLedBarGraph::OnPaint, this);
    Bind(wxEVT_ERASE_BACKGROUND, &wxLedBarGraph::OnEraseBackGround, this);
    Bind(wxEVT_SIZE, &wxLedBarGraph::OnResize, this);
    return true;
}

void wxLedBarGraph::OnEraseBackGround(wxEraseEvent&)
{
}

void wxLedBarGraph::OnResize(wxSizeEvent&)
{
    wxRect rect = GetClientRect();
    if (m_sizingMode == wxLED_BG_FIXED_BAR_COUNT) {
        if (m_orientation == wxHORIZONTAL)
            m_barWidths = (rect.width / m_nBars);
        else
            m_barWidths = (rect.height / m_nBars);

        if (m_barWidths < 6)
            return; // things less than 6 don't look that good, so you get a black box.
    } else {
        if (m_orientation == wxHORIZONTAL)
            m_nBars = rect.width / m_barWidths;
        else
            m_nBars = rect.height / m_barWidths;

        if ((m_nBars % 2) == 1)
            m_nBars--;
    }
    /*
        This figures out the gutters.  because of how i calculate width
        and how I draw the rects, there is some error that I center in the control...
    */
    if (m_orientation == wxHORIZONTAL)
        m_startX = (rect.width - (m_nBars * m_barWidths)) / 2;
    else
        m_startX = (rect.height - (m_nBars * m_barWidths)) / 2;

    Refresh();
}
/*
    I can't say I'm super happy with this function yet.
    There is still a lot of stuff in here that should not be recomputued on every
    paint or every paint call. However, it is a bit easier to understand what
    is happening and debug problems with it written this way
*/
void wxLedBarGraph::OnPaint(wxPaintEvent&)
{
    wxBufferedPaintDC dc(this);
    wxRect rect = GetClientRect();

    wxBrush blackBrush = wxBrush(wxColour("black"));
    wxColor tmpColor;

    int cntr = m_nBars / 2;
    int minG = 0; // = -1;
    int maxG = 0; // =  (m_nBars / 4);
    int minY = -1;
    int maxY = 1;

    if (m_mode == wxLED_BG_DOUBLE_SIDED) {
        minG = cntr - (m_nBars / 8);
        maxG = cntr + (m_nBars / 8);

        minY = cntr - 3 * (m_nBars / 8);
        maxY = cntr + 3 * (m_nBars / 8);
    } else if (m_mode == wxLED_BG_SINGLE_SIDED_TOP_LEFT) {
        minG = -1;
        maxG = (m_nBars / 4);

        minY = (m_nBars / 4) - 1;
        maxY = 3 * (m_nBars / 4);
    } else if (m_mode == wxLED_BG_SINGLE_SIDED_BOTTOM_RIGHT) {
        minG = 3 * (m_nBars / 4);
        maxG = m_nBars;

        minY = (m_nBars / 4) - 1;
        maxY = 3 * (m_nBars / 4);
    }
    int X = m_startX;

    //draw a black box for the first (left) gutter.
    dc.SetBrush(blackBrush);
    if (m_orientation == wxHORIZONTAL)
        dc.DrawRectangle(0, 0, X, rect.height);
    else
        dc.DrawRectangle(0, 0, rect.width, X);

    int thold = 0;
    int i;
    for (i = 0; i < m_nBars; i++) {
        //double dim = .7;
        bool maxedOut = false;
        bool drawMe = false;

        if (m_mode == wxLED_BG_DOUBLE_SIDED) {
            if (m_value < 0) {
                thold = (int)((m_nBars / 2) - (fabs(m_value) * (m_nBars / 2)));
                drawMe = (i >= thold) && (i <= (m_nBars / 2));

                if (m_value > -1)
                    maxedOut = false;
                else
                    maxedOut = true;

            } else if (m_value == 0) {
                drawMe = (i == m_nBars / 2);
                maxedOut = true;
            } else if (m_value > 0) {
                thold = (int)((m_nBars / 2) + (fabs(m_value) * (m_nBars / 2) + 1));
                drawMe = (i <= thold) && (i >= (m_nBars / 2));

                if (m_value < 1)
                    maxedOut = false;
                else
                    maxedOut = true;

            } else {
                // This state should be unreachable
                drawMe = false;
            }

        } else if (m_mode == wxLED_BG_SINGLE_SIDED_TOP_LEFT) {
            if (m_value <= -1) {
                if (i == 0) {
                    maxedOut = true;
                    drawMe = true; // ( i <= m_nBars );
                }
            } else if (m_value >= 1) {
                maxedOut = (i == m_nBars - 1);
                drawMe = true;
            } else {
                maxedOut = false;
                thold = (int)(m_nBars - ((m_nBars / 2) + (-m_value * (m_nBars / 2))));
                drawMe = (i <= thold);
            }
        } else if (m_mode == wxLED_BG_SINGLE_SIDED_BOTTOM_RIGHT) {
            if (m_value <= -1) {
                if (i == m_nBars - 1) {
                    drawMe = true;
                    maxedOut = true;
                }
            } else if (m_value >= 1) {
                maxedOut = (i == 0); //true;
                drawMe = true;
            } else {
                maxedOut = false;
                thold = (int)((m_nBars / 2) + (-m_value * (m_nBars / 2)));
                drawMe = (i >= thold);
            }
        }
        // I'd like to abstract out the color eventually
        if (i <= maxG && i >= minG)
            tmpColor = *wxGREEN;
        else if (i < maxY && i > minY)
            tmpColor = *wxYELLOW;
        else
            tmpColor = *wxRED;

        double dim = 0;
        //regular on
        if (!drawMe) {
            //led off
            dim = .15;
        } else if (maxedOut
                   && ((i == m_nBars - 1 || i == 0) || (i == m_nBars / 2))
                   && (m_mode == wxLED_BG_DOUBLE_SIDED)) {
            //max it out, dude!
            dim = 1;
        } else {
            dim = .7;
        };
        unsigned char r = tmpColor.Red();
        unsigned char g = tmpColor.Green();
        unsigned char b = tmpColor.Blue();
        tmpColor = wxColour((int)(r * dim), (int)(g * dim), (int)(b * dim));
        dc.SetBrush(wxBrush(tmpColor));

        if (m_orientation == wxHORIZONTAL)
            dc.DrawRectangle(X, 0, X + m_barWidths, rect.height);
        else
            dc.DrawRectangle(0, X, rect.width, X + m_barWidths);

        // Increment to the next bar
        X += m_barWidths;
    }
    // Draw the black gutter on the right edge
    dc.SetBrush(blackBrush);

    if (m_orientation == wxHORIZONTAL)
        dc.DrawRectangle(X, 0, rect.width, rect.height);
    else
        dc.DrawRectangle(0, X, rect.width, rect.height);
}

void wxLedBarGraph::SetMaxValue(double val)
{
    m_maxVal = val;
    RescaleValues();
}

void wxLedBarGraph::SetMinValue(double val)
{
    m_minVal = val;
    RescaleValues();
}

void wxLedBarGraph::SetValue(double val)
{
    m_realValue = val;
    RescaleValues();
}

void wxLedBarGraph::SetNBars(int nBars)
{
    m_nBars = nBars;

    wxSizeEvent b;
    OnResize(b);
}
void wxLedBarGraph::SetBarWidths(int width)
{
    m_barWidths = width;

    wxSizeEvent b;
    OnResize(b);
}

void wxLedBarGraph::SetSizingMode(wxLedBarGraphSizeMode mode)
{
    m_sizingMode = mode;

    wxSizeEvent a;
    OnResize(a);
}

void wxLedBarGraph::SetDrawingMode(wxLedBarGraphDrawMode mode)
{
    m_mode = mode;

    wxSizeEvent a;
    OnResize(a);
}

void wxLedBarGraph::SetOrientation(wxOrientation orient)
{
    m_orientation = orient;
#if 0
    InvalidateBestSize();
#endif
    wxSizeEvent a;
    OnResize(a);
}
