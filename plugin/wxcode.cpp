///////////////////////////////////////////////////////////////////////////////
//
// wxFormBuilder - A Visual Dialog Editor for wxWidgets.
// Copyright (C) 2005 José Antonio Hurtado
//
// This program is free software; you can redistribute it and/or
// modify it under the terms of the GNU General Public License
// as published by the Free Software Foundation; either version 2
// of the License, or (at your option) any later version.
//
// This program is distributed in the hope that it will be useful,
// but WITHOUT ANY WARRANTY; without even the implied warranty of
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
// GNU General Public License for more details.
//
// You should have received a copy of the GNU General Public License
// along with this program; if not, write to the Free Software
// Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA  02111-1307, USA.
//
// Written by
//   José Antonio Hurtado - joseantonio.hurtado@gmail.com
//   Juan Antonio Ortega  - jortegalalmolda@gmail.com
//
///////////////////////////////////////////////////////////////////////////////

#include "component.h"
#include "plugin.h"
#include "xrcconv.h"
#include "default.xpm"

#include <wxcode/plotctrl/plotctrl.h>
#include <wxcode/awx/led.h>
#include <wxcode/ledbargraph/ledbargraph.h>
#include <wxcode/treelistctrl/treelistctrl.h>

// wxFlatNotebook
#include <wx/wxFlatNotebook/wxFlatNotebook.h>
#include <wx/wxFlatNotebook/xh_fnb.h>

#include <math.h>

#include <wx/object.h>
#include <wx/image.h>
#include <wx/imaglist.h>
#include <wx/window.h>

///////////////////////////////////////////////////////////////////////////////

/**
Event handler for events generated by wxTreeListCtrl.
*/
class TreeListCtrlEvtHandler : public wxEvtHandler
{
public:
	TreeListCtrlEvtHandler( wxWindow* win, IManager* manager )
	:
	m_window( win ),
	m_manager( manager )
	{
	}

protected:
	void OnColClick( wxListEvent& event );
	void OnColEndDrag( wxListEvent& event );
	void OnLeftDClick( wxMouseEvent& event );

	DECLARE_EVENT_TABLE()
private:
	wxWindow* m_window;
	IManager* m_manager;

};

BEGIN_EVENT_TABLE( TreeListCtrlEvtHandler, wxEvtHandler )
	EVT_LIST_COL_CLICK( wxID_ANY, TreeListCtrlEvtHandler::OnColClick )
	EVT_LIST_COL_END_DRAG( wxID_ANY, TreeListCtrlEvtHandler::OnColEndDrag )
	EVT_LEFT_DCLICK( TreeListCtrlEvtHandler::OnLeftDClick )
END_EVENT_TABLE()

///////////////////////////////////////////////////////////////////////////////
class wxTreeListCtrlComponent : public ComponentBase
{
public:
	void AddImageList( IObject* obj, wxcode::wxTreeListCtrl* treeListCtrl )
	{
		if ( obj->IsNull( _("column_image_size") ) )
		{
			return;
		}

		wxSize imageSize = obj->GetPropertyAsSize(_("column_image_size"));
		if ( wxDefaultCoord == imageSize.GetWidth() || wxDefaultCoord == imageSize.GetHeight() )
		{
			return;
		}

		wxImageList* images = new wxImageList( imageSize.GetWidth(), imageSize.GetHeight() );
		wxImage image = wxBitmap( default_xpm ).ConvertToImage();
		images->Add( image.Scale( imageSize.GetWidth(), imageSize.GetHeight() ) );
		treeListCtrl->SetImageList( images );
	}

    wxObject* Create(IObject *obj, wxObject *parent)
    {
        wxcode::wxTreeListCtrl* treeListCtrl = new wxcode::wxTreeListCtrl( (wxWindow *)parent, -1,
                obj->GetPropertyAsPoint(_("pos")),
                obj->GetPropertyAsSize(_("size")),
                obj->GetPropertyAsInteger(_("style")) | obj->GetPropertyAsInteger(_("window_style")));

		// Add image list.
		AddImageList( obj, treeListCtrl );

		// Connect to event handler.
		treeListCtrl->PushEventHandler( new TreeListCtrlEvtHandler( treeListCtrl, GetManager() ) );

        return treeListCtrl;
    }

    void OnCreated( wxObject* wxobject, wxWindow* wxparent )
    {
		// initialize tree
		wxcode::wxTreeListCtrl* treeListCtrl = wxDynamicCast( wxobject, wxcode::wxTreeListCtrl );
		int colCount = treeListCtrl->GetColumnCount();

		// Check for columns
		if ( 0 == colCount )
		{
			return;
		}

        wxTreeItemId root = treeListCtrl->AddRoot (_("Root") );

        wxTreeItemId treeListParent;
        wxTreeItemId treeListItem;
        int n = 0;

		// Build tree.
		treeListItem = treeListCtrl->AppendItem( root, wxString::Format( _("Item #%d"), ++n ) );
		FillItem( treeListCtrl, treeListItem, colCount, n );

		treeListParent = treeListItem;
		for ( int i = 0; i < 5; ++i )
		{
			treeListItem = treeListCtrl->AppendItem( treeListParent, wxString::Format( _("Item #%d"), ++n ) );
			FillItem( treeListCtrl, treeListItem, colCount, n );
		}

		treeListParent = treeListItem;
		for ( int i = 0; i < 5; ++i )
		{
			treeListItem = treeListCtrl->AppendItem( treeListParent, wxString::Format( _("Item #%d"), ++n ) );
			FillItem( treeListCtrl, treeListItem, colCount, n );
		}

        treeListCtrl->ExpandAll( root );
    }

    void FillItem( wxcode::wxTreeListCtrl* treeListCtrl, wxTreeItemId itemId, int colCount, int row )
    {
    	for ( int i = 0; i < colCount; ++i )
		{
			treeListCtrl->SetItemText( itemId, i, wxString::Format( _("Item #%d, text #%d" ), row, i ) );
		}
    }
};

class wxTreeListCtrlColumnComponent : public ComponentBase
{
public:
	void OnCreated( wxObject* wxobject, wxWindow* wxparent )
	{
		// Easy read-only property access
		IObject* obj = GetManager()->GetIObject( wxobject );
		wxcode::wxTreeListCtrl* treeList = wxDynamicCast( wxparent, wxcode::wxTreeListCtrl );

		// Error checking
		if ( !( obj && treeList ) )
		{
			wxLogError( _("wxTreeListCtrlColumnComponent is missing its wxFormBuilder object(%i) or its parent(%i)"), obj,treeList );
			return;
		}

		// Apply image to page
		bool doImageAdd = false;
		wxImageList* imageList = NULL;
		IObject* parentObj = GetManager()->GetIObject( wxparent );
		if ( !parentObj->IsNull( _("column_image_size") ) )
		{
			if ( !obj->IsNull( _("image") ) )
			{
				wxSize imageSize = parentObj->GetPropertyAsSize( _("column_image_size") );
				int width = imageSize.GetWidth();
				int height = imageSize.GetHeight();
				if ( width > 0 && height > 0 )
				{
					imageList = treeList->GetImageList();
					if ( imageList != NULL )
					{
						wxImage image = obj->GetPropertyAsBitmap( _("image") ).ConvertToImage();
						imageList->Add( image.Scale( width, height ) );
						doImageAdd = true;
					}
				}
			}
		}

		treeList->AddColumn( obj->GetPropertyAsString( _("name") ),
			obj->GetPropertyAsInteger( _("width") ),
			obj->GetPropertyAsInteger( _("alignment") ),
			doImageAdd ? imageList->GetImageCount() - 1 : -1,
			obj->GetPropertyAsInteger( _("shown") ) != 0,
			obj->GetPropertyAsInteger( _("edit") ) !=0,
			obj->GetPropertyAsInteger( _("editor") ),
			obj->GetPropertyAsArrayString( _("choices") )
			);
	}

	void OnSelected( wxObject* wxobject )
	{

	}
};

void TreeListCtrlEvtHandler::OnColClick( wxListEvent& event )
{
	// Select the appropriate column
	wxcode::wxTreeListCtrl* tlc = wxDynamicCast( m_window, wxcode::wxTreeListCtrl );
	if ( tlc != NULL )
	{
		m_manager->SelectObject( m_manager->GetChild( m_window, event.GetColumn() ) );
	}
}

void TreeListCtrlEvtHandler::OnColEndDrag( wxListEvent& event )
{
	// Set the width property for the column.
	wxcode::wxTreeListCtrl* tlc = wxDynamicCast( m_window, wxcode::wxTreeListCtrl );
	if ( tlc != NULL )
	{
		wxObject* col = m_manager->GetChild( m_window, event.GetColumn() );
		m_manager->SelectObject( col );
		m_manager->ModifyProperty( col, _("width"),
			wxString::Format( wxT("%i"), tlc->GetColumnWidth( event.GetColumn() ) ), true );
	}
}

void TreeListCtrlEvtHandler::OnLeftDClick( wxMouseEvent& event )
{
	// Set the width property for the column.
	wxcode::wxTreeListCtrl* tlc = wxDynamicCast( m_window, wxcode::wxTreeListCtrl );
	if ( tlc != NULL )
	{
		int count = m_manager->GetChildCount( tlc );
		wxLogDebug( wxT("Child count: %i"), count );
		for ( int i =0; i < count; ++i )
		{
			wxObject* col = m_manager->GetChild( m_window, i );
			IObject* childObj = m_manager->GetIObject( col );
			int colWidth = tlc->GetColumnWidth( i );
			if ( childObj->GetPropertyAsInteger( _("width") ) != colWidth )
			{
				m_manager->ModifyProperty( col, _("width"),
					wxString::Format( wxT("%i"), colWidth ), true );
				m_manager->SelectObject( col );
				// One dclick means at most one column change.
				break;
			}
		}
	}
}

/**
Event handler for events generated by controls in this plugin
*/
class ComponentEvtHandler : public wxEvtHandler
{
private:
	wxWindow* m_window;
	IManager* m_manager;

public:
	ComponentEvtHandler( wxWindow* win, IManager* manager )
	:
	m_window( win ),
	m_manager( manager ) {}

protected:
	void OnFlatNotebookPageChanged( wxFlatNotebookEvent& event )
	{
		// Only handle events from this book - prevents problems with nested books, because OnSelected is fired on an
		// object and all of its parents
		if ( m_window != event.GetEventObject() )
		{
			return;
		}

		int selPage = event.GetSelection();
		if ( selPage < 0 )
		{
			return;
		}

		size_t count = m_manager->GetChildCount( m_window );
		for ( size_t i = 0; i < count; i++ )
		{
			wxObject* wxChild = m_manager->GetChild( m_window, i );
			IObject*  iChild = m_manager->GetIObject( wxChild );
			if ( iChild )
			{
				if ( (int)i == selPage && !iChild->GetPropertyAsInteger( _("select") ) )
				{
					m_manager->ModifyProperty( wxChild, _("select"), wxT("1"), false );
				}
				else if ( (int)i != selPage && iChild->GetPropertyAsInteger( _("select") ) )
				{
					m_manager->ModifyProperty( wxChild, _("select"), wxT("0"), false );
				}
			}
		}

		// Select the corresponding panel in the object tree
		wxFlatNotebook* book = wxDynamicCast( m_window, wxFlatNotebook );
		if ( NULL != book )
		{
			m_manager->SelectObject( book->GetPage( selPage ) );
		}
	}

	void OnFlatNotebookPageClosing( wxFlatNotebookEvent& event )
	{
		wxMessageBox( wxT("wxFlatNotebook pages can normally be closed.\nHowever, it is difficult to design a page that has been closed, so this action has been vetoed."),
						wxT("Page Close Vetoed!"), wxICON_INFORMATION, NULL );
		event.Veto();
	}

	DECLARE_EVENT_TABLE()
};

BEGIN_EVENT_TABLE( ComponentEvtHandler, wxEvtHandler )
	EVT_FLATNOTEBOOK_PAGE_CHANGED( -1, ComponentEvtHandler::OnFlatNotebookPageChanged )
	EVT_FLATNOTEBOOK_PAGE_CLOSING( -1, ComponentEvtHandler::OnFlatNotebookPageClosing )

END_EVENT_TABLE()

///////////////////////////////////////////////////////////////////////////////

class PlotCtrlComponent : public ComponentBase
{
public:
    wxObject* Create(IObject *obj, wxObject *parent)
    {
        // Set plot style
        wxPlotCtrl* plot = new wxPlotCtrl((wxWindow*)parent,-1,
                                          obj->GetPropertyAsPoint(_("pos")),
                                          obj->GetPropertyAsSize(_("size")) );

        plot->SetScrollOnThumbRelease( obj->GetPropertyAsInteger(_("scroll_on_thumb_release")) != 0 );
        plot->SetCrossHairCursor( obj->GetPropertyAsInteger( _("crosshair_cursor") ) != 0 );
        plot->SetDrawSymbols( obj->GetPropertyAsInteger( _("draw_symbols") ) != 0 );
        plot->SetDrawLines( obj->GetPropertyAsInteger( _("draw_lines") ) != 0 );
        plot->SetDrawSpline( obj->GetPropertyAsInteger( _("draw_spline") ) != 0 );
        plot->SetDrawGrid( obj->GetPropertyAsInteger( _("draw_grid") ) != 0 );

        plot->SetShowXAxis( obj->GetPropertyAsInteger( _("show_x_axis") ) != 0 );
        plot->SetShowYAxis( obj->GetPropertyAsInteger( _("show_y_axis") ) != 0 );
        plot->SetShowXAxisLabel( obj->GetPropertyAsInteger( _("show_x_axis_label") ) != 0 );
        plot->SetShowYAxisLabel( obj->GetPropertyAsInteger( _("show_y_axis_label") ) != 0 );
        plot->SetShowPlotTitle( obj->GetPropertyAsInteger( _("show_plot_title") ) != 0 );
        plot->SetShowKey( obj->GetPropertyAsInteger( _("show_key") ) != 0 );

        plot->SetAreaMouseFunction( (wxPlotCtrlMouse_Type)obj->GetPropertyAsInteger( _("area_mouse_function") ) );
        plot->SetAreaMouseMarker( (wxPlotCtrlMarker_Type)obj->GetPropertyAsInteger( _("area_mouse_marker") ) );

        if ( !obj->IsNull( _("grid_colour") ) )
        {
            plot->SetGridColour( obj->GetPropertyAsColour( _("grid_colour") ) );
        }
        if ( !obj->IsNull( _("border_colour") ) )
        {
            plot->SetBorderColour( obj->GetPropertyAsColour( _("border_colour") ) );
        }

        if ( !obj->IsNull( _("axis_font") ) )
        {
            plot->SetAxisFont( obj->GetPropertyAsFont( _("axis_font") ) );
        }
        if ( !obj->IsNull( _("axis_colour") ) )
        {
            plot->SetAxisColour( obj->GetPropertyAsColour( _("axis_colour") ) );
        }

        if ( !obj->IsNull( _("axis_label_font") ) )
        {
            plot->SetAxisLabelFont( obj->GetPropertyAsFont( _("axis_label_font") ) );
        }
        if ( !obj->IsNull( _("axis_label_colour") ) )
        {
            plot->SetAxisLabelColour( obj->GetPropertyAsColour( _("axis_label_colour") ) );
        }

        if ( !obj->IsNull( _("plot_title_font") ) )
        {
            plot->SetPlotTitleFont( obj->GetPropertyAsFont( _("plot_title_font") ) );
        }
        if ( !obj->IsNull( _("plot_title_colour") ) )
        {
            plot->SetPlotTitleColour( obj->GetPropertyAsColour( _("plot_title_colour") ) );
        }

        if ( !obj->IsNull( _("key_font") ) )
        {
            plot->SetKeyFont( obj->GetPropertyAsFont( _("key_font") ) );
        }
        if ( !obj->IsNull( _("key_colour") ) )
        {
            plot->SetKeyColour( obj->GetPropertyAsColour( _("key_colour") ) );
        }

        plot->SetXAxisLabel( obj->GetPropertyAsString( _("x_axis_label") ) );
        plot->SetYAxisLabel( obj->GetPropertyAsString( _("y_axis_label") ) );
        plot->SetPlotTitle( obj->GetPropertyAsString( _("plot_title") ) );
        plot->SetKeyPosition( obj->GetPropertyAsPoint( _("key_position") ) );

        wxPlotFunction plotFunc;
        plotFunc.Create( obj->GetPropertyAsString(_("sample_function")), wxT("x") );
        if ( plotFunc.Ok() )
        {
            plot->AddCurve( plotFunc, true, true );
        }

        return plot;

    }
    /*TiXmlElement* ExportToXrc(IObject *obj)
    {
    	ObjectToXrcFilter xrc(obj, _("wxPlotWindow"), obj->GetPropertyAsString(_("name")));
    	xrc.AddWindowProperties();
    	xrc.AddProperty(_("style"),_("style"), XRC_TYPE_BITLIST);
    	return xrc.GetXrcObject();
    }

    TiXmlElement* ImportFromXrc(TiXmlElement *xrcObj)
    {
    	XrcToXfbFilter filter(xrcObj, _("wxPlotWindow"));
    	filter.AddWindowProperties();
    	filter.AddProperty(_("style"),_("style"), XRC_TYPE_BITLIST);
    	return filter.GetXfbObject();
    }*/
};

class awxLedComponent : public ComponentBase
{
public:
    wxObject* Create(IObject *obj, wxObject *parent)
    {
        awxLed* led = new awxLed((wxWindow *)parent, -1,
                                 obj->GetPropertyAsPoint(_("pos")),
                                 obj->GetPropertyAsSize(_("size")),
                                 (awxLedColour)obj->GetPropertyAsInteger(_("color")),
                                 obj->GetPropertyAsInteger(_("window_style")));
        led->SetState( (awxLedState)obj->GetPropertyAsInteger(_("state")));
        return led;
    }
};

class wxLedBarGraphComponent : public ComponentBase
{
public:
    wxObject* Create(IObject *obj, wxObject *parent)
    {
        wxLedBarGraph* ledbg = new wxLedBarGraph((wxWindow *)parent, -1,
                obj->GetPropertyAsPoint(_("pos")),
                obj->GetPropertyAsSize(_("size")),
                obj->GetPropertyAsInteger(_("window_style")));

        ledbg->SetDrawingMode( (ledBGDrawMode) obj->GetPropertyAsInteger( _("drawing_mode") ));
        ledbg->SetSizingMode( (ledBGSizeMode) obj->GetPropertyAsInteger( _("sizing_mode") ));
        ledbg->SetOrientation( (ledBGOrient) obj->GetPropertyAsInteger( _("orientation") ));

        ledbg->SetBarWidths( obj->GetPropertyAsInteger( _("bar_widths") ));
        ledbg->SetNBars( obj->GetPropertyAsInteger( _("nbars") ));

        double max = obj->GetPropertyAsFloat( _("max_value"));
        double min = obj->GetPropertyAsFloat( _("min_value"));

        ledbg->SetMaxValue( max );
        ledbg->SetMinValue( min );

        double val = obj->GetPropertyAsFloat( _("value") );
        val = val* (max-min) + min;
        ledbg->SetValue( val );

        return ledbg;
    }
};

///////////////////////////////////////////////////////////////////////////////

class FlatNotebookComponent : public ComponentBase
{
public:
	wxObject* Create(IObject *obj, wxObject *parent)
	{
		wxFlatNotebook* book = new wxFlatNotebook((wxWindow *)parent,-1,
			obj->GetPropertyAsPoint(wxT("pos")),
			obj->GetPropertyAsSize(wxT("size")),
			obj->GetPropertyAsInteger(wxT("style")) | obj->GetPropertyAsInteger(wxT("window_style")));

		if ( obj->GetPropertyAsInteger( wxT("has_images") ) != 0 )
		{
			wxFlatNotebookImageList* images = new wxFlatNotebookImageList();
			book->SetImageList( images );
		}

		book->SetCustomizeOptions( obj->GetPropertyAsInteger( _("customize_options") ) );

		book->PushEventHandler( new ComponentEvtHandler( book, GetManager() ) );

		return book;
	}

	ticpp::Element* ExportToXrc(IObject *obj)
	{
		ObjectToXrcFilter xrc(obj, wxT("wxFlatNotebook"), obj->GetPropertyAsString(wxT("name")));
		xrc.AddWindowProperties();
		return xrc.GetXrcObject();
	}

	ticpp::Element* ImportFromXrc( ticpp::Element* xrcObj )
	{
		XrcToXfbFilter filter(xrcObj, wxT("wxFlatNotebook"));
		filter.AddWindowProperties();
		return filter.GetXfbObject();
	}
};

class FlatNotebookPageComponent : public ComponentBase
{
public:
	void OnCreated( wxObject* wxobject, wxWindow* wxparent )
	{
		// Easy read-only property access
		IObject* obj = GetManager()->GetIObject( wxobject );
		wxFlatNotebook* book = wxDynamicCast( wxparent, wxFlatNotebook );
		wxWindow* page = wxDynamicCast( GetManager()->GetChild( wxobject, 0 ), wxWindow );

		// Error checking
		if ( !( obj && book && page ) )
		{
			wxLogError( _("FlatNotebookPageComponent is missing its wxFormBuilder object(%i), its parent(%i), or its child(%i)"), obj, book, page );
			return;
		}

		// Prevent events during construction - two event handlers have been pushed onto the stack
		// VObjEvtHandler and Component Event handler
		wxEvtHandler* vobjEvtHandler = book->PopEventHandler();
		wxEvtHandler* bookEvtHandler = book->PopEventHandler();

		int selection = book->GetSelection();

		// Apply image to page
		IObject* parentObj = GetManager()->GetIObject( wxparent );
		if ( parentObj->GetPropertyAsInteger( wxT("has_images") ) != 0 )
		{
			if ( !obj->GetPropertyAsString( wxT("bitmap") ).empty() )
			{
				wxFlatNotebookImageList* imageList = book->GetImageList();
				if ( parentObj->GetPropertyAsInteger( wxT("auto_scale_images") ) != 0 )
				{
					wxImage image = obj->GetPropertyAsBitmap( wxT("bitmap") ).ConvertToImage();
					imageList->Add( image.Scale( 16, 16 ) );
				}
				else
				{
					imageList->Add( obj->GetPropertyAsBitmap( wxT("bitmap") ) );
				}
				book->AddPage( page, obj->GetPropertyAsString( wxT("label") ), false, imageList->GetCount() - 1 );
			}
			else
			{
				book->AddPage(page,obj->GetPropertyAsString(wxT("label")));
			}
		}
		else
		{
			book->AddPage(page,obj->GetPropertyAsString(wxT("label")));
		}

		if ( obj->GetPropertyAsString( wxT("select") ) == wxT("0") && selection >= 0 )
		{
			book->SetSelection( selection) ;
		}
		else
		{
			book->SetSelection( book->GetPageCount() - 1 );
		}

		// Restore event handling
		book->PushEventHandler( bookEvtHandler );
		book->PushEventHandler( vobjEvtHandler );
	}

	void OnSelected( wxObject* wxobject )
	{
		// Get actual page - first child
		wxObject* page = GetManager()->GetChild( wxobject, 0 );
		if ( NULL == page )
		{
			return;
		}

		wxFlatNotebook* book = wxDynamicCast( GetManager()->GetParent( wxobject ), wxFlatNotebook );
		if ( book )
		{
			for ( int i = 0; i < book->GetPageCount(); ++i )
			{
				if ( book->GetPage( i ) == page )
				{
					// Prevent infinite event loop
					wxEvtHandler* bookEvtHandler = book->PopEventHandler();
					wxEvtHandler* vobjEvtHandler = book->PopEventHandler();

					// Select Page
					book->SetSelection( i );

					// Restore event handling
					book->PushEventHandler( vobjEvtHandler );
					book->PushEventHandler( bookEvtHandler );
				}
			}
		}
	}

	ticpp::Element* ExportToXrc(IObject *obj)
	{
		ObjectToXrcFilter xrc( obj, wxT("notebookpage") );
		xrc.AddProperty( wxT("label"), wxT("label"), XRC_TYPE_TEXT );
		xrc.AddProperty( wxT("selected"), wxT("selected"), XRC_TYPE_BOOL );
		if ( !obj->IsNull( wxT("bitmap") ) )
		{
			xrc.AddProperty( wxT("bitmap"), wxT("bitmap"), XRC_TYPE_BITMAP );
		}
		return xrc.GetXrcObject();
	}

	ticpp::Element* ImportFromXrc( ticpp::Element* xrcObj )
	{
		XrcToXfbFilter filter( xrcObj, wxT("notebookpage") );
		filter.AddWindowProperties();
		filter.AddProperty( wxT("selected"), wxT("selected"), XRC_TYPE_BOOL );
		filter.AddProperty( wxT("label"), wxT("label"), XRC_TYPE_TEXT );
		filter.AddProperty( wxT("bitmap"), wxT("bitmap"), XRC_TYPE_BITMAP );
		return filter.GetXfbObject();
	}
};

///////////////////////////////////////////////////////////////////////////////

BEGIN_LIBRARY()

// wxPlotWindow
WINDOW_COMPONENT( "wxPlotCtrl",PlotCtrlComponent)
MACRO(wxPLOTCTRL_MOUSE_NOTHING)
MACRO(wxPLOTCTRL_MOUSE_ZOOM)
MACRO(wxPLOTCTRL_MOUSE_SELECT)
MACRO(wxPLOTCTRL_MOUSE_DESELECT)
MACRO(wxPLOTCTRL_MOUSE_PAN)
MACRO(wxPLOTCTRL_MARKER_NONE)
MACRO(wxPLOTCTRL_MARKER_RECT)
MACRO(wxPLOTCTRL_MARKER_VERT)
MACRO(wxPLOTCTRL_MARKER_HORIZ)

// awxLed
WINDOW_COMPONENT( "awxLed", awxLedComponent )
MACRO(awxLED_OFF)
MACRO(awxLED_ON)
MACRO(awxLED_BLINK)
MACRO(awxLED_LUCID)
MACRO(awxLED_RED)
MACRO(awxLED_GREEN)
MACRO(awxLED_YELLOW)

// wxLedBarGraph
WINDOW_COMPONENT( "wxLedBarGraph", wxLedBarGraphComponent )
MACRO(ledBG_DOUBLE_SIDED)
MACRO(ledBG_SINGLE_SIDED_TOP_LEFT)
MACRO(ledBG_SINGLE_SIDED_BOTTOM_RIGHT)
MACRO(ledBG_FIXED_N_BARS)
MACRO(ledBG_FIXED_BAR_SIZE)
MACRO(ledBG_ORIENT_VERTICAL)
MACRO(ledBG_ORIENT_HORIZONTAL)

// wxTreeListCtrl
WINDOW_COMPONENT( "wxcode::wxTreeListCtrl", wxTreeListCtrlComponent )
MACRO(wxTR_EDIT_LABELS)
MACRO(wxTR_NO_BUTTONS)
MACRO(wxTR_HAS_BUTTONS)
MACRO(wxTR_TWIST_BUTTONS)
MACRO(wxTR_NO_LINES)
MACRO(wxTR_FULL_ROW_HIGHLIGHT)
MACRO(wxTR_LINES_AT_ROOT)
MACRO(wxTR_HIDE_ROOT)
MACRO(wxTR_ROW_LINES)
MACRO(wxTR_HAS_VARIABLE_ROW_HEIGHT)
MACRO(wxTR_COLUMN_LINES)
MACRO(wxTR_SINGLE)
MACRO(wxTR_MULTIPLE)
MACRO(wxTR_EXTENDED)
MACRO(wxTR_DEFAULT_STYLE)
MACRO(wxTR_VIRTUAL)
lib->RegisterMacro( wxT("Text"), wxcode::wxTreeListColumnInfo::Text );
lib->RegisterMacro( wxT("TextInteger"), wxcode::wxTreeListColumnInfo::TextInteger );
lib->RegisterMacro( wxT("TextFloat"), wxcode::wxTreeListColumnInfo::TextFloat );
lib->RegisterMacro( wxT("TextAscii"), wxcode::wxTreeListColumnInfo::TextAscii );
lib->RegisterMacro( wxT("Combo"), wxcode::wxTreeListColumnInfo::Combo );
lib->RegisterMacro( wxT("Choice"), wxcode::wxTreeListColumnInfo::Choice );
lib->RegisterMacro( wxT("Spin"), wxcode::wxTreeListColumnInfo::Spin );

// wxTreeListCtrl
ABSTRACT_COMPONENT( "wxcode::wxTreeListCtrlColumn", wxTreeListCtrlColumnComponent )

// wxFlatNotebook
WINDOW_COMPONENT("wxFlatNotebook",FlatNotebookComponent)
ABSTRACT_COMPONENT("flatnotebookpage",FlatNotebookPageComponent)
MACRO(wxFNB_VC71)
MACRO(wxFNB_FANCY_TABS)
MACRO(wxFNB_TABS_BORDER_SIMPLE)
MACRO(wxFNB_NO_X_BUTTON)
MACRO(wxFNB_NO_NAV_BUTTONS)
MACRO(wxFNB_MOUSE_MIDDLE_CLOSES_TABS)
MACRO(wxFNB_BOTTOM)
MACRO(wxFNB_NODRAG)
MACRO(wxFNB_VC8)
MACRO(wxFNB_X_ON_TAB)
MACRO(wxFNB_BACKGROUND_GRADIENT)
MACRO(wxFNB_COLORFUL_TABS)
MACRO(wxFNB_DCLICK_CLOSES_TABS)
MACRO(wxFNB_SMART_TABS)
MACRO(wxFNB_DROPDOWN_TABS_LIST)
MACRO(wxFNB_ALLOW_FOREIGN_DND)
MACRO(wxFNB_FF2)
MACRO(wxFNB_CUSTOM_DLG)

// wxFNB Customizatio Options
MACRO(wxFNB_CUSTOM_TAB_LOOK)
MACRO(wxFNB_CUSTOM_ORIENTATION)
MACRO(wxFNB_CUSTOM_FOREIGN_DRAG)
MACRO(wxFNB_CUSTOM_LOCAL_DRAG)
MACRO(wxFNB_CUSTOM_CLOSE_BUTTON)
MACRO(wxFNB_CUSTOM_ALL)

END_LIBRARY()
