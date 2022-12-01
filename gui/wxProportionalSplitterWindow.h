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

/////////////////////////////////////////////////
/// \brief This class represents a splitter
/// window, which can controlled using a floating
/// point proportion instead of absolute values.
/////////////////////////////////////////////////
class ProportionalSplitterWindow : public wxSplitterWindow
{
        enum { MIN_PANE_SIZE = 1 };

    public:
        /////////////////////////////////////////////////
        /// \brief Default constructor.
        /////////////////////////////////////////////////
        ProportionalSplitterWindow() : wxSplitterWindow(), splitPercent_(0.5f), m_charHeight(0), m_defaultHeight(false)
        {}

        /////////////////////////////////////////////////
        /// \brief Construct a proportional splitter
        /// window using a set of parameters.
        ///
        /// \param parent wxWindow*
        /// \param id wxWindowID
        /// \param proportion float
        /// \param pos const wxPoint&
        /// \param size const wxSize&
        /// \param style long
        ///
        /////////////////////////////////////////////////
        ProportionalSplitterWindow(wxWindow* parent, wxWindowID id = wxID_ANY, float proportion = 0.5f,
                                   const wxPoint& pos = wxDefaultPosition, const wxSize& size = wxDefaultSize, long style = wxSP_3D)
            : wxSplitterWindow(parent, id, pos, size, style), splitPercent_(fabs(proportion))
        {
            wxASSERT_MSG( GetParent(), wxT("wxProportionalSplitterWindow parent window ptr cannot be null") );

            // Connect event handlers
            Connect(GetId(), wxEVT_SIZE, (wxObjectEventFunction)(wxEventFunction)&ProportionalSplitterWindow::OnSize);

            // Connect event handlers
            Connect(GetId(), wxEVT_COMMAND_SPLITTER_SASH_POS_CHANGED,
                    (wxObjectEventFunction)(wxEventFunction)&ProportionalSplitterWindow::OnSashPosChanged );

            // prevents double-click unsplit
            SetMinimumPaneSize(MIN_PANE_SIZE);

            if (proportion < 0)
                m_defaultHeight = true;
            else
                m_defaultHeight = false;

            m_charHeight = 10; // Arbitrary set size to 10px height;
            if ( splitPercent_ < 0.0 || splitPercent_ > 1.0 )
                splitPercent_ = 0.5f;

        }

        /////////////////////////////////////////////////
        /// \brief Split this window horizontally using
        /// the selected porportion.
        ///
        /// \param window1 wxWindow*
        /// \param window2 wxWindow*
        /// \param proportion float
        /// \return bool
        ///
        /////////////////////////////////////////////////
        virtual bool SplitHorizontally(wxWindow* window1, wxWindow* window2, float proportion = 0)
        {
            int splitSize = initSplitSize(proportion, true);
            return wxSplitterWindow::SplitHorizontally(window1, window2, splitSize);
        }

        /////////////////////////////////////////////////
        /// \brief Split this window vertically using the
        /// selected porportion.
        ///
        /// \param window1 wxWindow*
        /// \param window2 wxWindow*
        /// \param proportion float
        /// \return bool
        ///
        /////////////////////////////////////////////////
        virtual bool SplitVertically(wxWindow* window1, wxWindow* window2, float proportion = 0)
        {
            int splitSize = initSplitSize(proportion, false);
            return wxSplitterWindow::SplitVertically(window1, window2, splitSize);
        }

        /////////////////////////////////////////////////
        /// \brief Set the sash position using a floating
        /// point proportion.
        ///
        /// \param proportion float
        /// \return void
        ///
        /////////////////////////////////////////////////
        void SetSashPositionFloat(float proportion)
        {
            int splitSize = initSplitSize(proportion, isHorizontal());
            SetSashPosition(splitSize);
        }

        /////////////////////////////////////////////////
        /// \brief Handle the window size event. Will
        /// automatically adapt the sash position to
        /// achieve a multiple of a character height in
        /// window space.
        ///
        /// \param event wxSizeEvent&
        /// \return void
        ///
        /////////////////////////////////////////////////
        void OnSize(wxSizeEvent& event)
        {
            float parSize = parentSize();
            int nSashSize = GetSashSize();

            if (m_defaultHeight || splitPercent_ < 0)
            {
                if (rint((1.0-fabs(splitPercent_))*parSize / m_charHeight) <= 20)
                    SetSashPosition(parSize - rint((1.0-fabs(splitPercent_))*parSize / m_charHeight)*m_charHeight + nSashSize, false);
                else
                {
                    splitPercent_ = -(1.0 - 20 * m_charHeight / parSize);
                    SetSashPosition(parSize - 20 * m_charHeight + nSashSize, false);
                }
            }
            else
                SetSashPosition(parSize - rint((1.0-fabs(splitPercent_))*parSize / m_charHeight)*m_charHeight + nSashSize, false);

            event.Skip();
        }

        /////////////////////////////////////////////////
        /// \brief Handle the size event, when the sash
        /// position has been changed. Will automatically
        /// adapt the sash position to achieve a multiple
        /// of a character height in window space.
        ///
        /// \param event wxSplitterEvent&
        /// \return void
        ///
        /////////////////////////////////////////////////
        void OnSashPosChanged(wxSplitterEvent& event)
        {
            float parSize = parentSize();
            float percent = event.GetSashPosition() / parSize;

            if (percent > 0.0 && percent < 1.0)
                splitPercent_ = percent;

            m_defaultHeight = false;
            SetSashPosition(parSize-rint((1.0 - splitPercent_)*parSize / m_charHeight)*m_charHeight + GetSashSize(), false);

            event.Skip();
        }

        /////////////////////////////////////////////////
        /// \brief Return the current splitting
        /// proportion.
        ///
        /// \return float
        ///
        /////////////////////////////////////////////////
        float GetSplitPercentage()
        {
            return splitPercent_;
        }

        /////////////////////////////////////////////////
        /// \brief Change the character height in pixels.
        ///
        /// \param _charheight int
        /// \return void
        ///
        /////////////////////////////////////////////////
        void SetCharHeigth(int _charheight)
        {
            m_charHeight = _charheight;
        }

    private:

        /////////////////////////////////////////////////
        /// \brief Return true, if the splitter is
        /// horizontal.
        ///
        /// \return bool
        ///
        /////////////////////////////////////////////////
        bool isHorizontal() const
        {
            return (GetSplitMode() == wxSPLIT_HORIZONTAL);
        }

        /////////////////////////////////////////////////
        /// \brief Return the free space of the parent as
        /// a floating point number. Considers the
        /// orientation of the splitter.
        ///
        /// \return float
        ///
        /////////////////////////////////////////////////
        float parentSize() const
        {
            return (isHorizontal()
                    ? (float)GetParent()->GetClientSize().GetHeight()
                    : (float)GetParent()->GetClientSize().GetWidth());
        }

        /////////////////////////////////////////////////
        /// \brief Calculates the initial sash position
        /// after splitting the window.
        ///
        /// \param proportion float
        /// \param isHorizontal bool
        /// \return int
        ///
        /////////////////////////////////////////////////
        int initSplitSize(float proportion, bool isHorizontal)
        {
            SetSplitMode(isHorizontal ? wxSPLIT_HORIZONTAL : wxSPLIT_VERTICAL);

            // If the proportion is not equal to zero,
            // we will use it for the final proportion
            if (proportion != 0)
            {
                // Negative values will trigger the automatic
                // resizing mode
                if (proportion < 0)
                {
                    m_defaultHeight = true;
                    proportion = fabs(proportion);
                }
                else
                    m_defaultHeight = false;

                splitPercent_ = (proportion < 0.0 || proportion > 1.0) ? 0.5f : proportion;
            }
            else if (fabs(splitPercent_) > 1.0)
                splitPercent_ = 0.5f;

            // Get the free space of the parent
            int size = parentSize();

            // Use the automatic resizing mode or use the
            // passed proportion directly
            if (m_defaultHeight)
            {
                if (rint((1.0 - fabs(splitPercent_))*size / m_charHeight) <= 20)
                    return size - rint((1.0 - fabs(splitPercent_)) * size / m_charHeight) * m_charHeight + GetSashSize();
                else
                {
                    splitPercent_ = -(1.0 - 20 * m_charHeight / size);
                    return size - 20 * m_charHeight + GetSashSize();
                }
            }
            else
                return size - rint((1.0 - fabs(splitPercent_)) * size / m_charHeight) * m_charHeight + GetSashSize();
        }

    private:
        float splitPercent_;
        int m_charHeight;
        bool m_defaultHeight;
        DECLARE_NO_COPY_CLASS(ProportionalSplitterWindow)
};

#endif
