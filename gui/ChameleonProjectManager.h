#ifndef INTERFACE_MANAGER_H
#define INTERFACE_MANAGER_H


class EditorNotebook;

class ChameleonProjectManager
{
public:
	ChameleonProjectManager(EditorNotebook* notebook);


	void ClearDebugFocus();

private:
	EditorNotebook* m_book;
};




#endif
