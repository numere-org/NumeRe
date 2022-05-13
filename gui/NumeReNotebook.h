#ifndef CHAMELEON_NOTEBOOK__H
#define CHAMELEON_NOTEBOOK__H



//#include <wx/notebook.h>
#include <wx/aui/auibook.h>



class NumeReWindow;
class NumeReEditor;
class IconManager;

/////////////////////////////////////////////////
/// \brief This class represents the notebook
/// containing all editors of one main pane.
/////////////////////////////////////////////////
class EditorNotebook : public wxAuiNotebook
{
    public:
        EditorNotebook(wxWindow* parent, wxWindowID id,
                       IconManager* icons,
                       const wxPoint& pos = wxDefaultPosition,
                       const wxSize& size = wxDefaultSize, long style = 0);
        ~EditorNotebook();

        void SetTopParent(NumeReWindow* window)
            {m_top_parent = window;}

        void SetShowPathsOrIconsOnTabs(bool showText, bool showIcons);
        void SetTabText(size_t nTab, const wxString& text);
        NumeReEditor* createEditor(const wxString& text);
        NumeReEditor* getEditor(size_t pageNum, bool secondary = false);
        NumeReEditor* getCurrentEditor(bool secondary = false);
        NumeReEditor* getFocusedEditor();
        void split(size_t pageNum, bool horizontal);
        void unsplit(size_t pageNum);
        bool isSplit(size_t pageNum) const;

        void OnUnsplit(wxSplitterEvent& event);

        int FindPagePosition(wxNotebookPage* page);

        void OnButtonClicked (wxAuiNotebookEvent& event);
        void OnTabRightClicked (wxAuiNotebookEvent& event);
        void OnTabMove(wxAuiNotebookEvent& event);
        void OnTabMiddleClicked(wxAuiNotebookEvent& event);
        void OnTabScroll(wxMouseEvent& event);
        void OnEnter(wxMouseEvent& event);
        void OnLeave(wxMouseEvent& event);

        void OnSize(wxSizeEvent &event);

        bool GetMouseFocus() {return m_mouseFocus;}
        int GetTabFromPoint(const wxPoint& pt);

    private:
        IconManager* m_manager;
        NumeReWindow* m_top_parent;
        bool m_mouseFocus;
        bool m_showPathsOnTabs;
        bool m_showIconsOnTabs;


        DECLARE_EVENT_TABLE()
};

#endif
