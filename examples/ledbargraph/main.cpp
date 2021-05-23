#include "example.h"

#include <wx/app.h>

class Example : public wxApp {
public:
    virtual bool OnInit() override;

private:
    MainWindow* m_frame;
};

wxIMPLEMENT_APP(Example);

bool Example::OnInit()
{
    m_frame = new MainWindow();
    m_frame->Show();
    return true;
}
