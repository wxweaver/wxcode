#pragma once

#include <wx/dcbuffer.h>
#include <wx/wx.h>

enum wxLedBarGraphDrawMode {
    wxLED_BG_DOUBLE_SIDED = 0,
    wxLED_BG_SINGLE_SIDED_TOP_LEFT,
    wxLED_BG_SINGLE_SIDED_BOTTOM_RIGHT
};

enum wxLedBarGraphSizeMode {
    wxLED_BG_FIXED_BAR_COUNT = 0,
    wxLED_BG_FIXED_BAR_SIZE
};
/** Simple wxControl that simulates a LED Bar Graph with multiple colors
*/
class wxLedBarGraph : public wxControl {
public:
    /** Default constructor
    */
    wxLedBarGraph()
    {
        Init();
    }

    wxLedBarGraph(wxWindow* parent, wxWindowID id = wxID_ANY,
                  const wxPoint& pos = wxDefaultPosition,
                  const wxSize& size = wxDefaultSize,
                  long style = wxSUNKEN_BORDER,
                  const wxValidator& validator = wxDefaultValidator)
    {
        Init();
        Create(parent, id, pos, size, style, validator);
    }

    void SetSizingMode(wxLedBarGraphSizeMode mode);
    void SetDrawingMode(wxLedBarGraphDrawMode mode);
    void SetOrientation(wxOrientation orient);

    void SetMaxValue(double val);
    void SetMinValue(double val);
    void SetValue(double val);

    void SetBarCount(int barCount);
    void SetBarWidths(int width);

private:
    bool Create(wxWindow* parent, wxWindowID id = wxID_ANY,
                const wxPoint& pos = wxDefaultPosition,
                const wxSize& size = wxDefaultSize,
                long style = wxSUNKEN_BORDER,
                const wxValidator& validator = wxDefaultValidator);

    void Init()
    {
        m_barCount = 25;
        m_barWidths = 6;

        m_value = 0;
        m_maxVal = 1;
        m_minVal = -1;

        m_orientation = wxHORIZONTAL;
        m_sizingMode = wxLED_BG_FIXED_BAR_SIZE;
        m_mode = wxLED_BG_SINGLE_SIDED_TOP_LEFT;
    }

    wxSize DoGetBestSize() const
    {
        if (m_orientation == wxHORIZONTAL)
            return wxSize(155, 25);
        else
            return wxSize(25, 155);
    }

    // Force redraw when I get resized
    void OnResize(wxSizeEvent& evt);

    // Empty / prevent the drawing of a background.
    // This is combined with a double-buffered DC
    // to eliminate the annoying flashing on control update
    void OnEraseBackGround(wxEraseEvent& myEvent);

    // Draw the led bar graph
    void OnPaint(wxPaintEvent& evt);

    // Scale the internal value to (-1) -> (1)
    void RescaleValues()
    {
        double val = m_realValue - m_minVal;
        double span = (m_maxVal - m_minVal) / 2;

        m_value = (val - span) / span;
        this->Refresh();
    }
    int m_orientation;
    int m_mode;
    int m_sizingMode;

    // Only one of these works at a time, if the wrong sizingMode is selected,
    // the set methods will ignore your input.
    int m_barCount;  // Around 25 looks good most places
    int m_barWidths; // 6 looks good

    int m_startX;

    double m_maxVal;
    double m_minVal;
    double m_realValue;
    double m_value; // INTERNAL: always scaled to the range of -1 to 1.
                    // 0 is centered, -1 is the far left bar, 1 is the far right bar
                    // values > 1 or < -1 will trigger the "max-out" state,
                    // coloring the appropriate bar brighter

    wxDECLARE_DYNAMIC_CLASS(wxLedBarGraph);
};
