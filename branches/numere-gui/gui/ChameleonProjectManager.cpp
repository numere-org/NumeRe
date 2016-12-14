#include "ChameleonProjectManager.h"
#include "ChameleonNotebook.h"
#include "../editor/editor.h"

ChameleonProjectManager::ChameleonProjectManager( ChameleonNotebook* notebook )
{
	m_book = notebook;
}

void ChameleonProjectManager::ClearDebugFocus()
{
	// This is an absurdly naive way to handle removing the focused line
	// marker, but it's easy and shouldn't take up much time
	int numPages = m_book->GetPageCount();
	for(int i = 0; i < numPages; i++)
	{
		ChameleonEditor* pEdit = static_cast <ChameleonEditor* >(m_book->GetPage(i));
		pEdit->MarkerDeleteAll(MARKER_FOCUSEDLINE);
	}
}

