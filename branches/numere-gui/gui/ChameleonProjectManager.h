#ifndef INTERFACE_MANAGER_H
#define INTERFACE_MANAGER_H


class ChameleonNotebook;

class ChameleonProjectManager
{
public:
	ChameleonProjectManager(ChameleonNotebook* notebook);


	void ClearDebugFocus();

private:
	ChameleonNotebook* m_book;
};




#endif