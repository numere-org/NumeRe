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
#include "dtl/dtl.hpp"
#include <wx/zipstrm.h>
#include <wx/txtstrm.h>
#include <wx/wfstream.h>
#include <memory>
#include <fstream>
#include <sstream>
#include <wx/encconv.h>

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
/// \brief Converts a single-string file into a vector of strings
///
/// \param fileContent wxString
/// \return std::vector<wxString>
///
/// The file is split at the line break character so that each component
/// of the vector is a single line of the passed file
/////////////////////////////////////////////////
std::vector<wxString> FileRevisions::vectorize(wxString fileContent)
{
    std::vector<wxString> vVectorizedFile;

    // As long as there are contents left in the
    // current file
    while (fileContent.length())
    {
        // Push the current line to the vector
        vVectorizedFile.push_back(fileContent.substr(0, fileContent.find('\n')));

        // Erase the current line from the file
        if (fileContent.find('\n') != std::string::npos)
            fileContent.erase(0, fileContent.find('\n')+1);
        else
            break;
    }

    // Return the vectorized file
    return vVectorizedFile;
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
    wxFFileInputStream in(m_revisionPath.GetFullPath());
    wxZipInputStream zip(in);
    wxTextInputStream txt(zip, " \t", wxConvUTF8); // Convert from UTF-8 to Unicode on-the-fly

    std::unique_ptr<wxZipEntry> entry;

    // Search for the entry with the correct name
    // in the file
    while (entry.reset(zip.GetNextEntry()), entry.get() != nullptr)
    {
        // Found it?
        if (entry->GetName() == revString)
        {
            wxString revision;

            // Read the whole entry and append windows line endings. The
            // necessary conversion from UTF-8 is done by the text input
            // stream on-the-fly
            while (!zip.Eof())
                revision += wxString(txt.ReadLine()) + "\r\n";

            // Remove the last line's endings
            revision.erase(revision.length()-2);

            // If the ZIP comment equals "DIFF", we need to merge
            // the changes in this revision into the current
            // revision root
            if (entry->GetComment() == "DIFF")
                revision = createMerge(revision);

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
/// \brief This method returns the revision identifier of the last revision root.
///
/// \param revString const wxString&
/// \return wxString
///
/// The revision model is based upon comparing the new revisions
/// with a revision root and saving only the difference between
/// those two. The root revision may change upon time, because if
/// the difference between the root and the current item is larger
/// than the current item, the current item is the new revision root.
/// Revision root items are marked by the absence of any ZIP comment
/// except of the initial revision, which is the first root
/////////////////////////////////////////////////
wxString FileRevisions::getLastRevisionRoot(const wxString& revString)
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

        // Is there an empty comment or the initial revision?
        if (revFound
            && (revisionList[i].find("\tInitial revision") != std::string::npos
                || revisionList[i].substr(revisionList[i].length()-1) == "\t"))
            return revisionList[i].substr(0, revisionList[i].find('\t'));
    }

    return revString;
}


/////////////////////////////////////////////////
/// \brief This method calculates the differences between two files.
///
/// \param revision1 const wxString&
/// \param revisionID1 const wxString&
/// \param revision2 const wxString&
/// \param revisionID2 const wxString&
/// \return wxString
///
/// The file differences are created by the DTL libary found in
/// the "common" folder. The differences are returned in unified
/// format, i.e. containing sequences of changes starting with a
/// "@@ -a,b +c,d @@" section
/////////////////////////////////////////////////
wxString FileRevisions::diff(const wxString& revision1, const wxString& revisionID1, const wxString& revision2, const wxString& revisionID2)
{
    // Vectorize the revision root and the current file
    std::vector<wxString> vVectorizedRoot = vectorize(convertLineEndings(revision1));
    std::vector<wxString> vVectorizedFile = vectorize(convertLineEndings(revision2));

    // Calculate the differences between both files
    dtl::Diff<wxString> diffFile(vVectorizedRoot, vVectorizedFile);
    diffFile.compose();

    // Convert the differences into unified form
    diffFile.composeUnifiedHunks();

    // Print the differences to a string stream
    std::ostringstream uniDiff;

    // Get revision identifier
    if (revisionID1.length() && revisionID2.length())
        uniDiff << "--- " << revisionID1 << "\n+++ " << revisionID2 << "\n";

    diffFile.printUnifiedFormat(uniDiff);

    // Return the contents of the stream
    return uniDiff.str();
}


/////////////////////////////////////////////////
/// \brief This method creates the file differences between the file contents and the current revision root.
///
/// \param revisionContent const wxString&
/// \return wxString
///
/// The file differences are created in the private FileRevisions::diff()
/// method- The current revision root is detected automatically.
/////////////////////////////////////////////////
wxString FileRevisions::createDiff(const wxString& revisionContent)
{
    // Find the revision root
    wxString revisionRoot = getRevision(getLastRevisionRoot(getCurrentRevision()));

    // Calculate the differences of both files
    return diff(revisionRoot, getLastRevisionRoot(getCurrentRevision()), revisionContent, "rev"+toString(getRevisionCount()));
}


/////////////////////////////////////////////////
/// \brief This method merges the diff file into the current revision root.
///
/// \param diffFile const wxString&
/// \return wxString
///
/// The passed diff file is used to determine the revision root, which it is
/// based upon. Both files are vectorized first, then the changes noted in the
/// revision file are applied to the contents of the revision root, which is
/// merged back into a single-string file afterwards.
/////////////////////////////////////////////////
wxString FileRevisions::createMerge(const wxString& diffFile)
{
    // Vectorizes the passed diff file and the current revision root
    std::vector<wxString> vVectorizedDiff = vectorize(convertLineEndings(diffFile));
    std::vector<wxString> vVectorizedRoot = vectorize(convertLineEndings(getRevision(vVectorizedDiff.front().substr(4))));

    int nCurrentInputLine = 0;

    // Go through the lines of the diff file. Ignore the first
    // two, because they only contain the revision identifiers.
    for (size_t i = 2; i < vVectorizedDiff.size(); i++)
    {
        // The start of a change section. Extract the target input
        // line in the current revision (the second number set starting
        // with a "+")
        if (vVectorizedDiff[i].substr(0, 4) == "@@ -")
        {
            nCurrentInputLine = atoi(vVectorizedDiff[i].substr(vVectorizedDiff[i].find('+')+1, vVectorizedDiff[i].find(',', vVectorizedDiff[i].find('+') - vVectorizedDiff[i].find('+')-1)).c_str())-1;
            continue;
        }

        // A deletion. This requires that we decrement the current
        // input line
        if (vVectorizedDiff[i][0] == '-')
        {
            vVectorizedRoot.erase(vVectorizedRoot.begin()+nCurrentInputLine);
            nCurrentInputLine--;
        }

        // An addition
        if (vVectorizedDiff[i][0] == '+')
            vVectorizedRoot.insert(vVectorizedRoot.begin()+nCurrentInputLine, vVectorizedDiff[i].substr(1));

        // Always increment the current input line
        nCurrentInputLine++;
    }

    wxString mergedFile;

    // Merge the vectorized file into a single-string file
    for (size_t i = 0; i < vVectorizedRoot.size(); i++)
        mergedFile += vVectorizedRoot[i] + "\r\n";

    // Return the merged file with exception of the trailing
    // line break characters
    return mergedFile.substr(0, mergedFile.length()-2);
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
/// \brief This member function returns the maximal Diff file size.
///
/// \param nFileSize size_t The size of the stored file
/// \return size_t
///
/////////////////////////////////////////////////
size_t FileRevisions::getMaxDiffFileSize(size_t nFileSize)
{
    const size_t MINFILESIZE = 1024;
    double dOffset = nFileSize > MINFILESIZE ? ((double)MINFILESIZE / (double)nFileSize) : 0.5;
    size_t nTargetFileSize = ((1.0 - dOffset) * 0.8 * exp(-(double)getRevisionCount() / 40.0) + dOffset) * nFileSize;
    return nTargetFileSize > (MINFILESIZE / 2) ? nTargetFileSize : (MINFILESIZE / 2);
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
/// \brief This member function compares two
/// revisions with each other and returns the
/// differnces as unified diff.
///
/// \param rev1 const wxString&
/// \param rev2 const wxString&
/// \return wxString
///
/////////////////////////////////////////////////
wxString FileRevisions::compareRevisions(const wxString& rev1, const wxString& rev2)
{
    return diff(getRevision(rev1), rev1, getRevision(rev2), rev2);
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

        wxString diffFile = createDiff(revisionContent);

        if (diffFile.length() < getMaxDiffFileSize(revContent.length()))
            return createNewRevision(diffFile, "DIFF");
        else
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
    wxString currRev = getRevision(getCurrentRevision());
    if (currRev == revContent)
        return -1;

    // Ensure that the diff is not actually empty
    if (!diff(currRev, "", revContent, "").length())
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



