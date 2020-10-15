#ifndef ICONMANAGER_H
#define ICONMANAGER_H

#include <wx/string.h>
#include <wx/imaglist.h>

#include "../common/CommonHeaders.h"
#include "../common/datastructures.h"

class IconManager
{
public:
	IconManager(const wxString& programPath);
	~IconManager();

	int GetIconIndex(wxString iconInfo);
	wxImageList* GetImageList();

private:
	bool AddIconToList(wxString iconInfo);
	void CreateDisabledIcon(wxString iconInfo);

	IconManager(const IconManager&) = delete;
	IconManager& operator=(const IconManager&) = delete;


	StringIntHashmap m_iconExtensionMapping;
	wxImageList* m_images;

};


#endif
