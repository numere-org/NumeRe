#ifndef CHAMELEON_NOTEBOOK__H
#define CHAMELEON_NOTEBOOK__H



#include <wx/notebook.h>



class NumeReWindow;

class NumeReNotebook : public wxNotebook
{
public:
	NumeReNotebook(wxWindow* parent, wxWindowID id,
						const wxPoint& pos = wxDefaultPosition,
						const wxSize& size = wxDefaultSize, long style = 0,
						const wxString& name = "notebook");
	~NumeReNotebook();

	void SetTopParent(NumeReWindow* window)
        {m_top_parent = window;}

	//int HitTest(const wxPoint& pt, long& flags);
	int FindPagePosition(wxNotebookPage* page);

	void OnTabRightClicked (wxMouseEvent &event);
	void OnTabMiddleClicked(wxMouseEvent &event);

	void OnSize(wxSizeEvent &event);

private:
	//void CreateBookMenus();

	NumeReWindow* m_top_parent;


	DECLARE_EVENT_TABLE()
};

#endif
