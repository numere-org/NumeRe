#ifndef INTERFACE_MANAGER_H
#define INTERFACE_MANAGER_H


class NumeReNotebook;

class ChameleonProjectManager
{
public:
	ChameleonProjectManager(NumeReNotebook* notebook);


	void ClearDebugFocus();

private:
	NumeReNotebook* m_book;
};




#endif
