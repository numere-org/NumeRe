/*****************************************************************************
    NumeRe: Framework fuer Numerische Rechnungen
    Copyright (C) 2019  Erik Haenel et al.

    This program is free software: you can redistribute it and/or modify
    it under the terms of the GNU General Public License as published by
    the Free Software Foundation, either version 3 of the License, or
    (at your option) any later version.

    This program is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
    GNU General Public License for more details.

    You should have received a copy of the GNU General Public License
    along with this program.  If not, see <http://www.gnu.org/licenses/>.
******************************************************************************/

#include "filerevisions.hpp"
#include <wx/zipstrm.h>
#include <wx/txtstrm.h>
#include <wx/wfstream.h>
#include <memory>

#define COMPRESSIONLEVEL 9

std::string toString(size_t);

wxString FileRevisions::convertLineEndings(const wxString& content)
{
    wxString target = content;

    while (target.find("\r\n") != std::string::npos)
        target.erase(target.find("\r\n"), 1);

    return target;
}




FileRevisions::FileRevisions(const wxString& revisionPath) : m_revisionPath(revisionPath)
{
    if (!m_revisionPath.Exists())
        wxFileName::Mkdir(m_revisionPath.GetPath(), wxS_DIR_DEFAULT, wxPATH_MKDIR_FULL);
}


size_t FileRevisions::getRevisionCount()
{
    if (!m_revisionPath.Exists())
        return 0;

    wxFFileInputStream in(m_revisionPath.GetFullPath());
    wxZipInputStream zip(in);

    return zip.GetTotalEntries();
}


wxArrayString FileRevisions::getRevisionList()
{
    wxArrayString stringArray;

    if (m_revisionPath.Exists())
    {
        wxFFileInputStream in(m_revisionPath.GetFullPath());
        wxZipInputStream zip(in);

        std::unique_ptr<wxZipEntry> entry;

        while (entry.reset(zip.GetNextEntry()), entry.get() != nullptr)
        {
            stringArray.Add(entry->GetName() + "\t" + entry->GetDateTime().FormatISOCombined(' '));
        }
    }

    return stringArray;
}


wxString FileRevisions::getRevision(size_t nRevision)
{
    if (!m_revisionPath.Exists())
        return "";

    return getRevision("rev" + toString(nRevision));
}

wxString FileRevisions::getRevision(const wxString& revString)
{
    if (!m_revisionPath.Exists())
        return "";

    wxFFileInputStream in(m_revisionPath.GetFullPath());
    wxZipInputStream zip(in);
    wxTextInputStream txt(zip);

    std::unique_ptr<wxZipEntry> entry;

    while (entry.reset(zip.GetNextEntry()), entry.get() != nullptr)
    {
        if (entry->GetName() == revString);
        {
            wxString revision;

            while (!zip.Eof())
                revision += txt.ReadLine() + "\n";

            return revision;
        }
    }

    return "";
}


void FileRevisions::restoreRevision(size_t nRevision, const wxString& targetFile)
{
    wxString revision = getRevision(nRevision);

    wxFile restoredFile;
    restoredFile.Open(targetFile);
    restoredFile.Write(revision);
}


void FileRevisions::restoreRevision(const wxString& revString, const wxString& targetFile)
{
    wxString revision = getRevision(revString);

    wxFile restoredFile;
    restoredFile.Open(targetFile);
    restoredFile.Write(revision);
}


size_t FileRevisions::addRevision(const wxString& revisionContent)
{
    wxString revContent = convertLineEndings(revisionContent);

    if (!m_revisionPath.Exists())
    {
        wxFFileOutputStream out(m_revisionPath.GetFullPath());
        wxZipOutputStream zip(out, COMPRESSIONLEVEL);
        wxTextOutputStream txt(zip);

        wxZipEntry* rev0 = new wxZipEntry("rev0");
        rev0->SetComment("Initial revision");

        zip.PutNextEntry(rev0);
        txt << revContent;
        zip.CloseEntry();

        return 0;
    }
    else
    {
        size_t revisionNo = getRevisionCount();

        std::unique_ptr<wxFFileInputStream> in(new wxFFileInputStream(m_revisionPath.GetFullPath()));
        wxTempFileOutputStream out(m_revisionPath.GetFullPath());

        wxZipInputStream inzip(*in);
        wxZipOutputStream outzip(out, COMPRESSIONLEVEL);
        wxTextOutputStream txt(outzip);

        std::unique_ptr<wxZipEntry> entry;

        outzip.CopyArchiveMetaData(inzip);

        while (entry.reset(inzip.GetNextEntry()), entry.get() != nullptr)
        {
            outzip.CopyEntry(entry.release(), inzip);
        }

        wxZipEntry* currentRev = new wxZipEntry("rev" + toString(revisionNo));
        currentRev->SetComment("Revision created during saving");

        outzip.PutNextEntry(currentRev);
        txt << revContent;
        outzip.CloseEntry();
        in.reset();
        outzip.Close();
        out.Commit();

        return revisionNo;
    }
}


void FileRevisions::undoRevision()
{
    if (m_revisionPath.Exists())
    {
        size_t revisionNo = getRevisionCount();

        if (revisionNo > 1)
        {
            std::unique_ptr<wxFFileInputStream> in(new wxFFileInputStream(m_revisionPath.GetFullPath()));
            wxTempFileOutputStream out(m_revisionPath.GetFullPath());

            wxZipInputStream inzip(*in);
            wxZipOutputStream outzip(out, COMPRESSIONLEVEL);
            wxTextOutputStream txt(outzip);

            std::unique_ptr<wxZipEntry> entry;

            outzip.CopyArchiveMetaData(inzip);

            while (entry.reset(inzip.GetNextEntry()), entry.get() != nullptr)
            {
                if (entry->GetName() != "rev" + wxString(toString(revisionNo-1)))
                    outzip.CopyEntry(entry.release(), inzip);
            }

            in.reset();
            outzip.Close();
            out.Commit();
        }
        else
            wxRemoveFile(m_revisionPath.GetFullPath());
    }
}


