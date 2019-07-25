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
#include <fstream>

#define COMPRESSIONLEVEL 6

std::string toString(size_t);

/////////////////////////////////////////////////
/// \brief Constructor. Will try to create the missing folders on-the-fly.
///
/// \param revisionPath const wxString&
///
/////////////////////////////////////////////////
FileRevisions::FileRevisions(const wxString& revisionPath) : m_revisionPath(revisionPath)
{
    if (!m_revisionPath.Exists())
        wxFileName::Mkdir(m_revisionPath.GetPath(), wxS_DIR_DEFAULT, wxPATH_MKDIR_FULL);
}


/////////////////////////////////////////////////
/// \brief This method converts line end characters.
///
/// \param content const wxString&
/// \return wxString
///
/// Windows line ending characters (CR LF) are converted into
/// unix line endings (LF), which are used by the internal
/// ZIP file system
/////////////////////////////////////////////////
wxString FileRevisions::convertLineEndings(const wxString& content)
{
    wxString target = content;

    while (target.find("\r\n") != std::string::npos)
        target.erase(target.find("\r\n"), 1);

    return target;
}


/////////////////////////////////////////////////
/// \brief This method returns the contents of the selected revision.
///
/// \param revString const wxString&
/// \return wxString
///
/// The UTF8 multi-byte characters in the ZIP file are converted
/// automatically in wide chars during reading the file. Also,
/// the line ending characters are converted to windows line
/// endings (CR LF)
/////////////////////////////////////////////////
wxString FileRevisions::readRevision(const wxString& revString)
{
    wxMBConvUTF8 conv;
    wxFFileInputStream in(m_revisionPath.GetFullPath());
    wxZipInputStream zip(in);
    wxTextInputStream txt(zip);

    std::unique_ptr<wxZipEntry> entry;

    // Search for the entry with the correct name
    // in the file
    while (entry.reset(zip.GetNextEntry()), entry.get() != nullptr)
    {
        // Found it?
        if (entry->GetName() == revString)
        {
            wxString revision;

            // Read the whole entry, convert multibyte characters
            // into wide ones and append windows line endings
            while (!zip.Eof())
                revision += wxString(conv.cMB2WC(txt.ReadLine())) + "\r\n";

            // Remove the last line's endings
            revision.erase(revision.length()-2);

            return revision;
        }
    }

    return "";
}


/////////////////////////////////////////////////
/// \brief This method returns the last modification revision identifier.
///
/// \param revString const wxString&
/// \return wxString
///
/// The last content modification (no tag and no file move operation)
/// is returned. The algorithm starts his search from the passed revision
/// identifier string backwards in history.
/////////////////////////////////////////////////
wxString FileRevisions::getLastContentModification(const wxString& revString)
{
    // Get the revision list
    wxArrayString revisionList = getRevisionList();
    bool revFound = false;

    // Go reversely through the revision list and try to find
    // the revision tag, where the file was modified lastly
    for (int i = revisionList.size()-1; i >= 0; i--)
    {
        // Found the user selected revision?
        if (!revFound && revisionList[i].substr(0, revisionList[i].find('\t')) == revString)
            revFound = true;

        // Does the comment section contain one of the ignored
        // tags?
        if (revFound
            && revisionList[i].find("\tMOVE:") == std::string::npos
            && revisionList[i].find("\tRENAME:") == std::string::npos
            && revisionList[i].find("\tTAG:") == std::string::npos)
            return revisionList[i].substr(0, revisionList[i].find('\t'));
    }

    return revString;
}


/////////////////////////////////////////////////
/// \brief This method reads an external file into a string.
///
/// \param filePath const wxString&
/// \return wxString
///
/// The method is used to include external modifications into
/// the set of revisions, if these are actual modifications. The
/// file is read with windows line endings (CR LF)
/////////////////////////////////////////////////
wxString FileRevisions::readExternalFile(const wxString& filePath)
{
    std::ifstream file_in;
    wxString sFileContents;
    std::string sLine;
    wxMBConvUTF8 conv;

    file_in.open(filePath.ToStdString().c_str());

    while (file_in.good() && !file_in.eof())
    {
        std::getline(file_in, sLine);
        sFileContents += sLine + "\r\n";
    }

    sFileContents.erase(sFileContents.length()-2);

    return sFileContents;
}


/////////////////////////////////////////////////
/// \brief This method creates a new revision.
///
/// \param revContent const wxString&
/// \param comment const wxString&
/// \return size_t
///
/// The revision is appended to the already available list of
/// revisions at the end. The contents of the revision are
/// stored completely in the target ZIP file.
/////////////////////////////////////////////////
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

    // Copy all available entries to the new file
    while (entry.reset(inzip.GetNextEntry()), entry.get() != nullptr)
    {
        outzip.CopyEntry(entry.release(), inzip);
    }

    // Create the new revision
    wxZipEntry* currentRev = new wxZipEntry("rev" + toString(revisionNo));
    currentRev->SetComment(comment);

    outzip.PutNextEntry(currentRev);
    txt << revContent;
    outzip.CloseEntry();

    // Close the temporary file and commit it,
    // so that it may override the previous file
    // safely.
    in.reset();
    outzip.Close();
    out.Commit();

    return revisionNo;
}


/////////////////////////////////////////////////
/// \brief This method creates a new tag for the passed revision.
///
/// \param revString const wxString&
/// \param comment const wxString&
/// \return size_t
///
/// The new tag is appended directly below the corresponding
/// revision. After the new tag, the remaing parts of the file
/// are appended
/////////////////////////////////////////////////
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

    // Copy the contents to the new file
    while (entry.reset(inzip.GetNextEntry()), entry.get() != nullptr)
    {
        // is this the selected revision?
        if (entry->GetName() == revString)
        {
            outzip.CopyEntry(entry.release(), inzip);

            // Append the tag directly after the
            // selected revision
            outzip.PutNextEntry(taggedRevision);
            txt << revision;
            outzip.CloseEntry();
        }
        else
            outzip.CopyEntry(entry.release(), inzip);
    }

    // Close the temporary file and commit it,
    // so that it may override the previous file
    // safely.
    in.reset();
    outzip.Close();
    out.Commit();

    return revisionNo;
}


/////////////////////////////////////////////////
/// \brief This method handles all file move operations.
///
/// \param newRevPath const wxString&
/// \param comment const wxString&
/// \return void
///
/// File move operations are move and rename operations. Those are
/// reflected in the name and the location of the revisions file.
/////////////////////////////////////////////////
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


/////////////////////////////////////////////////
/// \brief Returns the number of available revisions.
///
/// \return size_t
///
/////////////////////////////////////////////////
size_t FileRevisions::getRevisionCount()
{
    if (!m_revisionPath.Exists())
        return 0;

    wxFFileInputStream in(m_revisionPath.GetFullPath());
    wxZipInputStream zip(in);

    return zip.GetTotalEntries();
}


/////////////////////////////////////////////////
/// \brief This method returns a log-like list of revisions.
///
/// \return wxArrayString
///
/// The returned string array can be used to display a log of
/// all revisions done to the corresponding file. The list
/// contains actual revisions, tags and file move operations.
/// The list contains date-time and comments of the revisions.
/// These are tab-separated from the revision identifiers.
/////////////////////////////////////////////////
wxArrayString FileRevisions::getRevisionList()
{
    wxArrayString stringArray;

    if (m_revisionPath.Exists())
    {
        wxFFileInputStream in(m_revisionPath.GetFullPath());
        wxZipInputStream zip(in);

        std::unique_ptr<wxZipEntry> entry;

        // Get the names, the time and the comments for each entry
        // in the ZIP file
        while (entry.reset(zip.GetNextEntry()), entry.get() != nullptr)
        {
            stringArray.Add(entry->GetName() + "\t" + entry->GetDateTime().FormatISOCombined(' ') + "\t" + entry->GetComment());
        }
    }

    return stringArray;
}


/////////////////////////////////////////////////
/// \brief This method returns the revision identifier of the current revision.
///
/// \return wxString
///
/// It excludes all tags, which are attached to the current
/// revision, because they do not contain any file changes.
/////////////////////////////////////////////////
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


/////////////////////////////////////////////////
/// \brief Returns the contents of the selected revision.
///
/// \param nRevision size_t
/// \return wxString
///
/// If the selected revision is not a content modification
/// revision, the algorithm will search for the last previous
/// revision containing file changes.
/////////////////////////////////////////////////
wxString FileRevisions::getRevision(size_t nRevision)
{
    if (!m_revisionPath.Exists())
        return "";

    return getRevision("rev" + toString(nRevision));
}


/////////////////////////////////////////////////
/// \brief Returns the contents of the selected revision.
///
/// \param revString wxString
/// \return wxString
///
/// If the selected revision is not a content modification
/// revision, the algorithm will search for the last previous
/// revision containing file changes.
/////////////////////////////////////////////////
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


/////////////////////////////////////////////////
/// \brief This method will restore the contents of the selected revision.
///
/// \param nRevision size_t
/// \param targetFile const wxString&
/// \return void
///
/// The contents of the revision are written to the file, which has been
/// selected as second argument.
/////////////////////////////////////////////////
void FileRevisions::restoreRevision(size_t nRevision, const wxString& targetFile)
{
    wxString revision = getRevision(nRevision);

    wxFile restoredFile;
    restoredFile.Open(targetFile, wxFile::write);
    restoredFile.Write(revision);
}


/////////////////////////////////////////////////
/// \brief This method will restore the contents of the selected revision.
///
/// \param revString const wxString&
/// \param targetFile const wxString&
/// \return void
///
/// The contents of the revision are written to the file, which has been
/// selected as second argument.
/////////////////////////////////////////////////
void FileRevisions::restoreRevision(const wxString& revString, const wxString& targetFile)
{
    wxString revision = getRevision(revString);

    wxFile restoredFile;
    restoredFile.Open(targetFile, wxFile::write);
    restoredFile.Write(revision);
}


/////////////////////////////////////////////////
/// \brief Allows the user to tag a selected revision with the passed comment
///
/// \param nRevision size_t
/// \param tagComment const wxString&
/// \return size_t
///
/////////////////////////////////////////////////
size_t FileRevisions::tagRevision(size_t nRevision, const wxString& tagComment)
{
    // Will probably fail, because the selected revision is not
    // stored as "revXYZ" but as "tagABC-revXYZ"
    return createNewTag("rev" + toString(nRevision), tagComment);
}


/////////////////////////////////////////////////
/// \brief Allows the user to tag a selected revision with the passed comment
///
/// \param revString const wxString&
/// \param tagComment const wxString&
/// \return size_t
///
/////////////////////////////////////////////////
size_t FileRevisions::tagRevision(const wxString& revString, const wxString& tagComment)
{
    return createNewTag(revString, tagComment);
}


/////////////////////////////////////////////////
/// \brief This method adds a new revision.
///
/// \param revisionContent const wxString&
/// \return size_t
///
/// If there's no revisions file yet, this method will create
/// a new one containing the revisions content's as so-called
/// "initial revision".
/////////////////////////////////////////////////
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
        // Only add a new revision if the contents differ
        if (revisionContent == getRevision(getCurrentRevision()))
            return -1;

        return createNewRevision(revContent, "");
    }
}


/////////////////////////////////////////////////
/// \brief This method adds an external modification as new revision.
///
/// \param filePath const wxString&
/// \return size_t
///
/// The revision is only appended, if its a real file contents
/// change. Therefore the content of the external modification
/// is compared to the current revision available in the list and
/// only appended, if they are different. This will avoid creating
/// revisions resulting from file meta data updates.
/////////////////////////////////////////////////
size_t FileRevisions::addExternalRevision(const wxString& filePath)
{
    wxString revContent = readExternalFile(filePath);

    if (!revContent.length())
        revContent = "Other error";

    // Only add the external revision, if it actually
    // modified the file
    if (getRevision(getCurrentRevision()) == revContent)
        return -1;

    return createNewRevision(convertLineEndings(revContent), "External modification");
}


/////////////////////////////////////////////////
/// \brief This method removes the last added revision.
///
/// \return void
///
/////////////////////////////////////////////////
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


/////////////////////////////////////////////////
/// \brief This method handles renames of the corresponding file.
///
/// \param oldName const wxString&
/// \param newName const wxString&
/// \param newRevPath const wxString&
/// \return void
///
/// This change is reflected on the revisions file, which is also
/// renamed as well.
/////////////////////////////////////////////////
void FileRevisions::renameFile(const wxString& oldName, const wxString& newName, const wxString& newRevPath)
{
    fileMove(newRevPath, "RENAME: " + oldName + " -> " + newName);
}


/////////////////////////////////////////////////
/// \brief This method handles moves of the corresponding file.
///
/// \param oldPath const wxString&
/// \param newPath const wxString&
/// \param newRevPath const wxString&
/// \return void
///
/// This change is reflected on the revisions file, which is also
/// moved as well.
/////////////////////////////////////////////////
void FileRevisions::moveFile(const wxString& oldPath, const wxString& newPath, const wxString& newRevPath)
{
    fileMove(newRevPath, "MOVE: " + oldPath + " -> " + newPath);
}



