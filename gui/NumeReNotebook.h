#ifndef CHAMELEON_NOTEBOOK__H
#define CHAMELEON_NOTEBOOK__H



#include <wx/notebook.h>



class NumeReWindow;

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
    void AddNewTab(wxWindow* window, const wxString& text, bool select);

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
