#ifndef CHAMELEON_NOTEBOOK__H
#define CHAMELEON_NOTEBOOK__H



#include <wx/notebook.h>



class NumeReWindow;
class NumeReEditor;

/////////////////////////////////////////////////
/// \brief This class represents the notebook
/// containing all editors of one main pane.
/////////////////////////////////////////////////
class EditorNotebook : public wxNotebook
{
public:
	EditorNotebook(wxWindow* parent, wxWindowID id,
						const wxPoint& pos = wxDefaultPosition,
						const wxSize& size = wxDefaultSize, long style = 0,
						const wxString& name = "notebook");
	~EditorNotebook();

	void SetTopParent(NumeReWindow* window)
        {m_top_parent = window;}

    void SetShowPathsOnTabs(bool showText);
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

	void OnTabRightClicked (wxMouseEvent &event);
	void OnTabMiddleClicked(wxMouseEvent &event);
	void OnTabScroll(wxMouseEvent& event);
	void OnEnter(wxMouseEvent& event);
	void OnLeave(wxMouseEvent& event);

	void OnSize(wxSizeEvent &event);

	bool GetMouseFocus() {return m_mouseFocus;}

private:

	NumeReWindow* m_top_parent;
	bool m_mouseFocus;
	bool m_showPathsOnTabs;


	DECLARE_EVENT_TABLE()
};

#endif
