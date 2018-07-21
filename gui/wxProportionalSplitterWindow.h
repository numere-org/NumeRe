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
#include <cmath>

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
		splitPercent_(fabs(proportion))
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

		if (proportion < 0)
            m_defaultHeight = true;
        else
            m_defaultHeight = false;

        m_charHeight = 10; // Arbitrary set size to 10px height;
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
		int splitSize = initSplitSize(proportion, false);
		return wxSplitterWindow::SplitVertically( window1,
			window2, splitSize );
	}

	void OnSize(wxSizeEvent& event)
	{
        if (m_defaultHeight || splitPercent_ < 0)
        {
            if (rint((1.0-fabs(splitPercent_))*parentSize()/m_charHeight) <= 20)
                SetSashPosition( static_cast<int>(parentSize()-rint((1.0-fabs(splitPercent_))*parentSize()/m_charHeight)*m_charHeight)+GetSashSize(), false);
            else
            {
                splitPercent_ = -(1.0-20*m_charHeight/parentSize());
                SetSashPosition(static_cast<int>(parentSize()-20*m_charHeight+GetSashSize()), false);
            }
		}
		else
            SetSashPosition( static_cast<int>(parentSize()-rint((1.0-fabs(splitPercent_))*parentSize()/m_charHeight)*m_charHeight)+GetSashSize(), false );
		event.Skip();
	}

	void OnSashPosChanged(wxSplitterEvent& event)
	{
		float percent =  (float)event.GetSashPosition() / parentSize();
		if ( percent > 0.0 && percent < 1.0 )
			splitPercent_ = percent;
        m_defaultHeight = false;

		SetSashPosition( static_cast<int>(parentSize()-rint((1.0-splitPercent_)*parentSize()/m_charHeight)*m_charHeight)+GetSashSize(), false );
		//SetSashPosition( static_cast<int>(splitPercent_ * parentSize()), false );
		event.Skip();
	}

	float GetSplitPercentage() { return splitPercent_; }
	void SetCharHeigth(int _charheight) {m_charHeight = _charheight;}

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
            if (proportion < 0)
            {
                m_defaultHeight = true;
                proportion = fabs(proportion);
            }
            else
                m_defaultHeight = false;
			if ( proportion < 0.0 || proportion > 1.0 )
				splitPercent_ = 0.5f;
			else
				splitPercent_ = proportion;
		}
		else if (fabs(splitPercent_) > 1.0)
		{
			splitPercent_ = 0.5f;
		}

		int size = parentSize();
		if (m_defaultHeight)
		{
            if (rint((1.0-fabs(splitPercent_))*size/m_charHeight) <= 20)
                return static_cast<int>(size-rint((1.0-fabs(splitPercent_))*size/m_charHeight)*m_charHeight+GetSashSize());
            else
            {
                splitPercent_ = -(1.0-20*m_charHeight/size);
                return static_cast<int>(size-20*m_charHeight+GetSashSize());
            }
        }
        else
            return static_cast<int>(size-rint((1.0-fabs(splitPercent_))*size/m_charHeight)*m_charHeight+GetSashSize());
	}
private:
	float splitPercent_;
	int m_charHeight;
	bool m_defaultHeight;
	DECLARE_NO_COPY_CLASS(wxProportionalSplitterWindow)
};

#endif
