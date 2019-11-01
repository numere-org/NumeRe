#include "ChameleonProjectManager.h"
#include "NumeReNotebook.h"
#include "editor/editor.h"

ChameleonProjectManager::ChameleonProjectManager( EditorNotebook* notebook )
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
		NumeReEditor* pEdit = static_cast <NumeReEditor* >(m_book->GetPage(i));
		pEdit->MarkerDeleteAll(MARKER_FOCUSEDLINE);
	}
}

