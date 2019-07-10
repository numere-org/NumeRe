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

#define COMPRESSIONLEVEL 6

std::string toString(size_t);

FileRevisions::FileRevisions(const wxString& revisionPath) : m_revisionPath(revisionPath)
{
    if (!m_revisionPath.Exists())
        wxFileName::Mkdir(m_revisionPath.GetPath(), wxS_DIR_DEFAULT, wxPATH_MKDIR_FULL);
}


wxString FileRevisions::convertLineEndings(const wxString& content)
{
    wxString target = content;

    while (target.find("\r\n") != std::string::npos)
        target.erase(target.find("\r\n"), 1);

    return target;
}


wxString FileRevisions::readRevision(const wxString& revString)
{
    wxMBConvUTF8 conv;
    wxFFileInputStream in(m_revisionPath.GetFullPath());
    wxZipInputStream zip(in);
    wxTextInputStream txt(zip);

    std::unique_ptr<wxZipEntry> entry;

    while (entry.reset(zip.GetNextEntry()), entry.get() != nullptr)
    {
        if (entry->GetName() == revString)
        {
            wxString revision;

            while (!zip.Eof())
                revision += wxString(conv.cMB2WC(txt.ReadLine())) + "\n";

            return revision;
        }
    }

    return "";
}


wxString FileRevisions::getLastContentModification(const wxString& revString)
{
    wxArrayString revisionList = getRevisionList();
    bool revFound = false;

    for (int i = revisionList.size()-1; i >= 0; i--)
    {
        if (!revFound && revisionList[i].substr(0, revisionList[i].find('\t')) == revString)
            revFound = true;

        if (revFound
            && revisionList[i].find("\tMOVE:") == std::string::npos
            && revisionList[i].find("\tRENAME:") == std::string::npos
            && revisionList[i].find("\tTAG:") == std::string::npos)
            return revisionList[i].substr(0, revisionList[i].find('\t'));
    }

    return revString;
}


size_t FileRevisions::createNewRevision(const wxString& revContent, const wxString& comment)
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
    currentRev->SetComment(comment);

    outzip.PutNextEntry(currentRev);
    txt << revContent;
    outzip.CloseEntry();
    in.reset();
    outzip.Close();
    out.Commit();

    return revisionNo;
}


size_t FileRevisions::createNewTag(const wxString& revString, const wxString& comment)
{
    wxString revision = "TAG FOR " + revString;
    size_t revisionNo = getRevisionCount();
    wxZipEntry* taggedRevision = new wxZipEntry("tag" + revString.substr(3) + "-rev" + toString(revisionNo));
    taggedRevision->SetComment("TAG: " + comment);

    std::unique_ptr<wxFFileInputStream> in(new wxFFileInputStream(m_revisionPath.GetFullPath()));
    wxTempFileOutputStream out(m_revisionPath.GetFullPath());

    wxZipInputStream inzip(*in);
    wxZipOutputStream outzip(out, COMPRESSIONLEVEL);
    wxTextOutputStream txt(outzip);

    std::unique_ptr<wxZipEntry> entry;

    outzip.CopyArchiveMetaData(inzip);

    while (entry.reset(inzip.GetNextEntry()), entry.get() != nullptr)
    {
        if (entry->GetName() == revString)
        {
            outzip.CopyEntry(entry.release(), inzip);
            outzip.PutNextEntry(taggedRevision);
            txt << revision;
            outzip.CloseEntry();
        }
        else
            outzip.CopyEntry(entry.release(), inzip);
    }

    in.reset();
    outzip.Close();
    out.Commit();

    return revisionNo;
}


void FileRevisions::fileMove(const wxString& newRevPath, const wxString& comment)
{
    wxString revContent = "FILEMOVE OPERATION ON " + getCurrentRevision();
    createNewRevision(revContent, comment);

    wxFileName newPath(newRevPath);

    if (!newPath.DirExists())
        wxFileName::Mkdir(newPath.GetPath(), wxS_DIR_DEFAULT, wxPATH_MKDIR_FULL);

    wxRenameFile(m_revisionPath.GetFullPath(), newRevPath);
    m_revisionPath.Assign(newRevPath);
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
            stringArray.Add(entry->GetName() + "\t" + entry->GetDateTime().FormatISOCombined(' ') + "\t" + entry->GetComment());
        }
    }

    return stringArray;
}


wxString FileRevisions::getCurrentRevision()
{
    wxArrayString revList = getRevisionList();

    for (int i = revList.size()-1; i >= 0; i--)
    {
        if (revList[i].substr(0, 3) == "rev")
            return revList[i].substr(0, revList[i].find('\t'));
    }

    return "";
}


wxString FileRevisions::getRevision(size_t nRevision)
{
    if (!m_revisionPath.Exists())
        return "";

    return getRevision("rev" + toString(nRevision));
}


wxString FileRevisions::getRevision(wxString revString)
{
    if (!m_revisionPath.Exists())
        return "";

    if (revString.substr(0, 3) == "tag")
    {
        revString.replace(0, 3, "rev");
        revString.erase(revString.find('-'));
    }

    revString = getLastContentModification(revString);

    return readRevision(revString);
}


void FileRevisions::restoreRevision(size_t nRevision, const wxString& targetFile)
{
    wxString revision = getRevision(nRevision);

    wxFile restoredFile;
    restoredFile.Open(targetFile, wxFile::write);
    restoredFile.Write(revision);
}


void FileRevisions::restoreRevision(const wxString& revString, const wxString& targetFile)
{
    wxString revision = getRevision(revString);

    wxFile restoredFile;
    restoredFile.Open(targetFile, wxFile::write);
    restoredFile.Write(revision);
}


size_t FileRevisions::tagRevision(size_t nRevision, const wxString& tagComment)
{
    // Will probably fail, because the selected revision is not
    // stored as "revXYZ" but as "tagABC-revXYZ"
    return createNewTag("rev" + toString(nRevision), tagComment);
}


size_t FileRevisions::tagRevision(const wxString& revString, const wxString& tagComment)
{
    return createNewTag(revString, tagComment);
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
        return createNewRevision(revContent, "Saving revision");
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


void FileRevisions::renameFile(const wxString& oldName, const wxString& newName, const wxString& newRevPath)
{
    fileMove(newRevPath, "RENAME: " + oldName + " -> " + newName);
}


void FileRevisions::moveFile(const wxString& oldPath, const wxString& newPath, const wxString& newRevPath)
{
    fileMove(newRevPath, "MOVE: " + oldPath + " -> " + newPath);
}



