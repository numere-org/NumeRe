#include "IconManager.h"

#include <string>

#include <wx/icon.h>
#include <wx/iconloc.h>
#include <wx/mimetype.h>
#include <wx/bitmap.h>

#include "dialogs/defaultfile.xpm"
#include "dialogs/closedfolder16x1632bpp.xpm"
//#include "dialogs/openfolder16x1632bpp.xpm"
#include "dialogs/exe.xpm"


IconManager::IconManager(const wxString& programPath)
{
	m_images = new wxImageList(16, 16);


	m_iconExtensionMapping["cpj"] = m_images->GetImageCount();
	m_iconExtensionMapping["DEFAULTFILEEXTENSION"] = m_images->GetImageCount();
	wxBitmap defaultfile(defaultfile_xpm);
	m_images->Add(defaultfile);

	wxBitmap closedfolder(closedfolder16x1632bpp_xpm);
	m_iconExtensionMapping["FOLDERCLOSED"] = m_images->GetImageCount();
	m_images->Add(closedfolder);

	//wxBitmap openfolder(openfolder16x1632bpp_xpm);
	wxIcon openfolder(programPath + "\\icons\\folder.ico", wxBITMAP_TYPE_ICO, 16,16);
	m_iconExtensionMapping["FOLDEROPEN"] = m_images->GetImageCount();
	m_images->Add(openfolder);

	wxIcon functions(programPath + "\\icons\\fnc.ico", wxBITMAP_TYPE_ICO, 16,16);
	m_iconExtensionMapping["FUNCTIONS"] = m_images->GetImageCount();
	m_images->Add(functions);

	wxIcon commands(programPath + "\\icons\\cmd.ico", wxBITMAP_TYPE_ICO, 16,16);
	m_iconExtensionMapping["COMMANDS"] = m_images->GetImageCount();
	m_images->Add(commands);

	wxIcon NSCR(programPath + "\\icons\\nscr.ico", wxBITMAP_TYPE_ICO, 16,16);
	m_iconExtensionMapping["nscr"] = m_images->GetImageCount();
	m_iconExtensionMapping[".nscr"] = m_images->GetImageCount();
	m_images->Add(NSCR);

	wxIcon ICON(programPath + "\\icons\\icon.ico", wxBITMAP_TYPE_ICO, 16,16);
	m_iconExtensionMapping["WORKPLACE"] = m_images->GetImageCount();
	m_images->Add(ICON);

	wxIcon NPRC(programPath + "\\icons\\nprc.ico", wxBITMAP_TYPE_ICO, 16,16);
	m_iconExtensionMapping["nprc"] = m_images->GetImageCount();
	m_iconExtensionMapping[".nprc"] = m_images->GetImageCount();
	m_images->Add(NPRC);

	wxIcon NDAT(programPath + "\\icons\\ndat.ico", wxBITMAP_TYPE_ICO, 16,16);
	m_iconExtensionMapping["ndat"] = m_images->GetImageCount();
	m_iconExtensionMapping[".ndat"] = m_images->GetImageCount();
	m_images->Add(NDAT);

	wxIcon DAT(programPath + "\\icons\\dat.ico", wxBITMAP_TYPE_ICO, 16,16);
	m_iconExtensionMapping["dat"] = m_images->GetImageCount();
	m_iconExtensionMapping[".dat"] = m_images->GetImageCount();
	m_images->Add(DAT);

	wxIcon JDX(programPath + "\\icons\\jdx.ico", wxBITMAP_TYPE_ICO, 16,16);
	m_iconExtensionMapping["jdx"] = m_images->GetImageCount();
	m_iconExtensionMapping["dx"] = m_images->GetImageCount();
	m_iconExtensionMapping["jcm"] = m_images->GetImageCount();
	m_iconExtensionMapping[".jdx"] = m_images->GetImageCount();
	m_iconExtensionMapping[".dx"] = m_images->GetImageCount();
	m_iconExtensionMapping[".jcm"] = m_images->GetImageCount();
	m_images->Add(JDX);

	wxIcon LABX(programPath + "\\icons\\labx.ico", wxBITMAP_TYPE_ICO, 16,16);
	m_iconExtensionMapping["labx"] = m_images->GetImageCount();
	m_iconExtensionMapping[".labx"] = m_images->GetImageCount();
	m_images->Add(LABX);

	wxIcon IBW(programPath + "\\icons\\ibw.ico", wxBITMAP_TYPE_ICO, 16,16);
	m_iconExtensionMapping["ibw"] = m_images->GetImageCount();
	m_iconExtensionMapping[".ibw"] = m_images->GetImageCount();
	m_images->Add(IBW);



	AddIconToList("c");
	AddIconToList("cpp");
	AddIconToList("h");

	m_iconExtensionMapping["hpp"] = GetIconIndex("h");
	AddIconToList("lib");

	CreateDisabledIcon("c");
	CreateDisabledIcon("cpp");
	CreateDisabledIcon("h");

	AddIconToList("txt");

	wxBitmap exe(exe_xpm);
	m_iconExtensionMapping["exe"] = m_images->GetImageCount();

	// we'll assume any .out files are also executable
	m_iconExtensionMapping["out"] = m_images->GetImageCount();
	m_images->Add(exe);
}

IconManager::~IconManager()
{
	delete m_images;
}

bool IconManager::AddIconToList(wxString iconInfo)
{
	// wxTheMimeTypesManager is a wxWidgets-created global instance
	wxFileType* fileType = wxTheMimeTypesManager->GetFileTypeFromExtension(iconInfo);

	if(fileType == NULL)
	{
		return false;
	}

	wxIconLocation iconLocation;

	bool result = false;
	if(fileType->GetIcon(&iconLocation))
	{
		wxIcon fileIcon;

		wxString fullname = iconLocation.GetFileName();

		if(fullname == wxEmptyString || fullname == "%1")
		{
			return false;
		}

		if ( iconLocation.GetIndex() )
		{
			fullname << _T(';') << iconLocation.GetIndex();
		}

        while (fullname.find('"') != std::string::npos)
            fullname.erase(fullname.find('"'),1);
		fileIcon.LoadFile(fullname, wxBITMAP_TYPE_ICO, 16, 16);

		int newIconIndex = m_images->GetImageCount();
		m_images->Add(fileIcon);

		m_iconExtensionMapping[iconInfo] = newIconIndex;

		result = true;
	}

	delete fileType;
	return result;
}

int IconManager::GetIconIndex(wxString iconInfo)
{
	int currentExtensionIconNumber;

	if(iconInfo == wxEmptyString)
	{
		return m_iconExtensionMapping["DEFAULTFILEEXTENSION"];
	}

	if(m_iconExtensionMapping.find(iconInfo) != m_iconExtensionMapping.end())
	{
		currentExtensionIconNumber = m_iconExtensionMapping[iconInfo];
	}
	else
	{
		if(AddIconToList(iconInfo))
		{
			currentExtensionIconNumber = m_iconExtensionMapping[iconInfo];
		}
		else
		{
			currentExtensionIconNumber = m_iconExtensionMapping["DEFAULTFILEEXTENSION"];
		}
	}

	return currentExtensionIconNumber;
}


void IconManager::CreateDisabledIcon(wxString iconInfo)
{
	int iconIndex = GetIconIndex(iconInfo);
	wxIcon icon = m_images->GetIcon(iconIndex);
	wxBitmap iconBitmap;
	iconBitmap.CopyFromIcon(icon);

	wxMemoryDC dc;
	wxPen pen(wxColour("navy"), 2);
	dc.SetPen(pen);

	dc.SelectObject(iconBitmap);
	dc.DrawLine(0, 0, 15, 15);
	dc.DrawLine(15, 0, 0, 15);
	dc.SelectObject(wxNullBitmap);

	// TODO: copying back to an icon may be unnecessary.
	wxIcon disabledIcon;
	disabledIcon.CopyFromBitmap(iconBitmap);

	wxString disabledIconName = iconInfo + "_disabled";
	m_iconExtensionMapping[disabledIconName] = m_images->GetImageCount();
	m_images->Add(disabledIcon);
}

wxImageList* IconManager::GetImageList()
{
	return m_images;
}
