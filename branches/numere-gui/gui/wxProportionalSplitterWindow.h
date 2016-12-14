#ifndef PROPORTIONALSPLITTER
#define PROPORTIONALSPLITTER

//*****************************************************************************
// wxProportionalSplitterWindow
//*****************************************************************************

#include <wx/wxprec.h>
#ifdef __BORLANDC__
#pragma hdrstop
#endif
#ifndef WX_PRECOMP
//here the list of needed .h files that are included in wx/wx.h
#endif

#include <wx/splitter.h>

class WXDLLEXPORT wxProportionalSplitterWindow : public wxSplitterWindow
{
	enum { MIN_PANE_SIZE = 1 };
public:
	// Default constructor
	wxProportionalSplitterWindow() : wxSplitterWindow(), splitPercent_(0.5f)
	{}

	// Normal constructor
	wxProportionalSplitterWindow(wxWindow *parent, 
		wxWindowID id = wxID_ANY,
		float proportion = 0.5f,
		const wxPoint& pos = wxDefaultPosition,
		const wxSize& size = wxDefaultSize,
		long style = wxSP_3D,
		const wxString& name = wxT("proportional_splitter"))
		: wxSplitterWindow(parent, id, pos, size, style, name),
		splitPercent_(proportion)
	{
		wxASSERT_MSG( GetParent(), wxT("wxProportionalSplitterWindow parent window ptr cannot be null") );

		Connect( GetId(),
			wxEVT_SIZE, 
			(wxObjectEventFunction)(wxEventFunction)&wxProportionalSplitterWindow::OnSize );

		Connect( GetId(),
			wxEVT_COMMAND_SPLITTER_SASH_POS_CHANGED, 
			(wxObjectEventFunction)(wxEventFunction)&wxProportionalSplitterWindow::OnSashPosChanged );


		// prevents double-click unsplit
		SetMinimumPaneSize( MIN_PANE_SIZE );

		if ( splitPercent_ < 0.0 || splitPercent_ > 1.0 )
			splitPercent_ = 0.5f;

	}

	virtual bool SplitHorizontally(wxWindow *window1,
		wxWindow *window2,
		float proportion = 0)
	{
		int splitSize = initSplitSize(proportion, true);
		return wxSplitterWindow::SplitHorizontally( window1, 
			window2, splitSize);
	}

	virtual bool SplitVertically(wxWindow *window1,
		wxWindow *window2,
		float proportion = 0)
	{
		int splitSize = initSplitSize(proportion, true);
		return wxSplitterWindow::SplitVertically( window1, 
			window2, splitSize );
	}

	void OnSize(wxSizeEvent& event)
	{
		SetSashPosition( static_cast<int>(splitPercent_ * parentSize()), false );
		event.Skip();
	}

	void OnSashPosChanged(wxSplitterEvent& event)
	{
		float percent =  (float)event.GetSashPosition() / parentSize();
		if ( percent > 0.0 || percent < 1.0 )
			splitPercent_ =  percent;

		SetSashPosition( static_cast<int>(splitPercent_ * parentSize()), false );
		event.Skip();
	}

	float GetSplitPercentage() { return splitPercent_; }

private:
	bool isHorizontal(void) const { return (GetSplitMode() == wxSPLIT_HORIZONTAL); }

	float parentSize(void) const
	{
		return (isHorizontal() ? (float)GetParent()->GetClientSize().GetHeight()
			: (float)GetParent()->GetClientSize().GetWidth() );
	}

	int initSplitSize(float proportion, bool isHorizontal)
	{
		SetSplitMode(isHorizontal ? wxSPLIT_HORIZONTAL : wxSPLIT_VERTICAL);

		if ( proportion != 0 )
		{
			if ( proportion < 0.0 || proportion > 1.0 )
				splitPercent_ = 0.5f;
			else
				splitPercent_ = proportion;
		}
		else if ( splitPercent_ < 0.0 || splitPercent_ > 1.0 )
		{
			splitPercent_ = 0.5f;
		}

		int size = parentSize();
		return static_cast<int>(splitPercent_ * size);
	}
private:
	float splitPercent_;
	DECLARE_NO_COPY_CLASS(wxProportionalSplitterWindow)
};

#endif