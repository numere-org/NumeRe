#include "IconManager.h"
#include "globals.hpp"

#include <string>
#include <vector>

#include <wx/icon.h>
#include <wx/iconloc.h>
#include <wx/mimetype.h>
#include <wx/bitmap.h>

#include "icons/newfile.xpm"
#include "icons/closedfolder16x1632bpp.xpm"
#include "icons/exe.xpm"
#include "icons/doc.xpm"

#include "../kernel/core/io/logger.hpp"




IconManager::IconManager(const wxString& programPath)
{
    m_imageScaleFactor = 1.0;

	m_images = new wxImageList(16, 16);


	m_iconExtensionMapping["cpj"] = m_images->GetImageCount();
	m_iconExtensionMapping["DEFAULTFILEEXTENSION"] = m_images->GetImageCount();
	wxBitmap defaultfile(newfile_xpm);
	m_images->Add(defaultfile);

	wxBitmap closedfolder(closedfolder16x1632bpp_xpm);
	m_iconExtensionMapping["FOLDERCLOSED"] = m_images->GetImageCount();
	m_images->Add(closedfolder);

	wxBitmap document(doc_xpm);
	m_iconExtensionMapping["DOCUMENT"] = m_images->GetImageCount();
	m_iconExtensionMapping["txt"] = m_images->GetImageCount();
	m_iconExtensionMapping[".txt"] = m_images->GetImageCount();
	m_iconExtensionMapping["log"] = m_images->GetImageCount();
	m_iconExtensionMapping[".log"] = m_images->GetImageCount();
	m_iconExtensionMapping["xml"] = m_images->GetImageCount();
	m_iconExtensionMapping[".xml"] = m_images->GetImageCount();
	m_iconExtensionMapping["md"] = m_images->GetImageCount();
	m_iconExtensionMapping[".md"] = m_images->GetImageCount();
	m_images->Add(document);

	//wxBitmap openfolder(openfolder16x1632bpp_xpm);
	wxIcon openfolder(programPath + "/icons/folder.ico", wxBITMAP_TYPE_ICO);
	m_iconExtensionMapping["FOLDEROPEN"] = m_images->GetImageCount();
	m_images->Add(openfolder);

	wxIcon functions(programPath + "/icons/fnc.ico", wxBITMAP_TYPE_ICO);
	m_iconExtensionMapping["FUNCTIONS"] = m_images->GetImageCount();
	m_images->Add(functions);

	wxIcon commands(programPath + "/icons/cmd.ico", wxBITMAP_TYPE_ICO);
	m_iconExtensionMapping["COMMANDS"] = m_images->GetImageCount();
	m_images->Add(commands);

	wxIcon constants(programPath + "/icons/cnst.ico", wxBITMAP_TYPE_ICO);
	m_iconExtensionMapping["CONSTANTS"] = m_images->GetImageCount();
	m_images->Add(constants);

	wxIcon methods(programPath + "/icons/mthd.png", wxBITMAP_TYPE_PNG);
	m_iconExtensionMapping["METHODS"] = m_images->GetImageCount();
	m_images->Add(methods);

	wxIcon NSCR(programPath + "/icons/nscr.ico", wxBITMAP_TYPE_ICO);
	m_iconExtensionMapping["nscr"] = m_images->GetImageCount();
	m_iconExtensionMapping[".nscr"] = m_images->GetImageCount();
	m_images->Add(NSCR);

	wxIcon ICON(programPath + "/icons/icon.ico", wxBITMAP_TYPE_ICO);
	m_iconExtensionMapping["WORKPLACE"] = m_images->GetImageCount();
	m_images->Add(ICON);

	wxIcon NPRC(programPath + "/icons/nprc.ico", wxBITMAP_TYPE_ICO);
	m_iconExtensionMapping["nprc"] = m_images->GetImageCount();
	m_iconExtensionMapping[".nprc"] = m_images->GetImageCount();
	m_images->Add(NPRC);

	wxIcon NDAT(programPath + "/icons/ndat.ico", wxBITMAP_TYPE_ICO);
	m_iconExtensionMapping["ndat"] = m_images->GetImageCount();
	m_iconExtensionMapping[".ndat"] = m_images->GetImageCount();
	m_images->Add(NDAT);

	wxIcon NLYT(programPath + "/icons/nlyt.ico", wxBITMAP_TYPE_ICO);
	m_iconExtensionMapping["nlyt"] = m_images->GetImageCount();
	m_iconExtensionMapping[".nlyt"] = m_images->GetImageCount();
	m_images->Add(NLYT);

	wxIcon NPKP(programPath + "/icons/npkp.ico", wxBITMAP_TYPE_ICO);
	m_iconExtensionMapping["npkp"] = m_images->GetImageCount();
	m_iconExtensionMapping[".npkp"] = m_images->GetImageCount();
	m_images->Add(NPKP);

	wxIcon NHLP(programPath + "/icons/nhlp.ico", wxBITMAP_TYPE_ICO);
	m_iconExtensionMapping["nhlp"] = m_images->GetImageCount();
	m_iconExtensionMapping[".nhlp"] = m_images->GetImageCount();
	m_images->Add(NHLP);

	wxIcon DAT(programPath + "/icons/dat.ico", wxBITMAP_TYPE_ICO);
	m_iconExtensionMapping["dat"] = m_images->GetImageCount();
	m_iconExtensionMapping[".dat"] = m_images->GetImageCount();
	m_images->Add(DAT);

	wxIcon JDX(programPath + "/icons/jdx.ico", wxBITMAP_TYPE_ICO);
	m_iconExtensionMapping["jdx"] = m_images->GetImageCount();
	m_iconExtensionMapping["dx"] = m_images->GetImageCount();
	m_iconExtensionMapping["jcm"] = m_images->GetImageCount();
	m_iconExtensionMapping[".jdx"] = m_images->GetImageCount();
	m_iconExtensionMapping[".dx"] = m_images->GetImageCount();
	m_iconExtensionMapping[".jcm"] = m_images->GetImageCount();
	m_images->Add(JDX);

	wxIcon LABX(programPath + "/icons/labx.ico", wxBITMAP_TYPE_ICO);
	m_iconExtensionMapping["labx"] = m_images->GetImageCount();
	m_iconExtensionMapping[".labx"] = m_images->GetImageCount();
	m_images->Add(LABX);

	wxIcon IBW(programPath + "/icons/ibw.ico", wxBITMAP_TYPE_ICO);
	m_iconExtensionMapping["ibw"] = m_images->GetImageCount();
	m_iconExtensionMapping[".ibw"] = m_images->GetImageCount();
	m_images->Add(IBW);

	wxIcon PDF(programPath + "/icons/pdf.ico", wxBITMAP_TYPE_ICO);
	m_iconExtensionMapping["pdf"] = m_images->GetImageCount();
	m_iconExtensionMapping[".pdf"] = m_images->GetImageCount();
	m_images->Add(PDF);

	wxIcon MATLAB(programPath + "/icons/m.ico", wxBITMAP_TYPE_ICO);
	m_iconExtensionMapping["m"] = m_images->GetImageCount();
	m_iconExtensionMapping[".m"] = m_images->GetImageCount();
	m_images->Add(MATLAB);

	AddIconToList("c");
	AddIconToList("cpp");
	AddIconToList("h");

	m_iconExtensionMapping["hpp"] = GetIconIndex("h");
	AddIconToList("lib");

	CreateDisabledIcon("c");
	CreateDisabledIcon("cpp");
	CreateDisabledIcon("h");

	wxBitmap exe(exe_xpm);
	m_iconExtensionMapping["exe"] = m_images->GetImageCount();
}

IconManager::~IconManager()
{
	delete m_images;
}

bool IconManager::AddIconToList(wxString iconInfo)
{
	// wxTheMimeTypesManager is a wxWidgets-created global instance
	wxFileType* fileType = wxTheMimeTypesManager->GetFileTypeFromExtension(iconInfo);

	if (fileType == NULL)
	{
		return false;
	}

	wxIconLocation iconLocation;

	bool result = false;

	if (fileType->GetIcon(&iconLocation))
	{
		wxIcon fileIcon;
		wxString fullname = iconLocation.GetFileName();

        while (fullname.find('"') != std::string::npos)
            fullname.erase(fullname.find('"'),1);

		if (fullname == wxEmptyString || fullname == "%1" || fullname == "%2")
		{
		    m_iconExtensionMapping[iconInfo] = m_iconExtensionMapping["DEFAULTFILEEXTENSION"];
			return true;
		}

		if (iconLocation.GetIndex())
			fullname << _T(';') << iconLocation.GetIndex();

        bool res = false;

		if (!(res = fileIcon.LoadFile(fullname, wxBITMAP_TYPE_ICO, std::rint(16*m_imageScaleFactor), std::rint(16*m_imageScaleFactor))))
        {
            // Hack-try-detect-DPIs
            if (m_imageScaleFactor == 1.0)
            {
                // Somewhat the most common scaling factors
                std::vector<double> scaleFactors = {1.2, 1.25, 1.4, 1.5, 1.75, 1.8, 2.0};

                // Try to find the fitting factor
                for (auto scale : scaleFactors)
                {
                    if ((res = fileIcon.LoadFile(fullname, wxBITMAP_TYPE_ICO, std::rint(16*scale), std::rint(16*scale))))
                    {
                        m_imageScaleFactor = scale;
                        g_pixelScale = scale;
                        break;
                    }
                }
            }

            if (!res)
                return false;
        }

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

	if (iconInfo == wxEmptyString)
	{
		return m_iconExtensionMapping["DEFAULTFILEEXTENSION"];
	}

	if (m_iconExtensionMapping.find(iconInfo) != m_iconExtensionMapping.end())
	{
		currentExtensionIconNumber = m_iconExtensionMapping[iconInfo];
	}
	else
	{
		if (AddIconToList(iconInfo))
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
