#define CRTDBG_MAP_ALLOC
#include <stdlib.h>
#ifdef _MSC_VER
    #include <crtdbg.h>
#else
    #define _ASSERT(expr) ((void)0)

    #define _ASSERTE(expr) ((void)0)
#endif

#include "ProjectInfo.h"
#include "../editor/editor.h"
#include "debug.h"

#ifdef _DEBUG
#define new DEBUG_NEW
#endif

ProjectInfo::ProjectInfo(bool singleFile /* = true */)
{
	m_isSingleFile = singleFile;
	m_isCompiled = false;
	m_isReadOnly = false;
	//m_relativePaths = true;
	// isRemote will be set by the creator immediately upon instantiation
	// in fact, it could almost go in the constructor...
	m_isRemote = true;
	m_isBeingCompiled = false;
	m_isCompilable = false;
	m_projectName = "Chameleon"; // <-- ha ha, just a default

	if(singleFile) {
		//set some defaults
		//m_headerFiles = wxArrayString;
		//m_sourceFiles = wxArrayString;
		//m_libraryFiles = wxArrayString;
		//m_headersEnabled = BoolArray;
		//m_sourcesEnabled = BoolArray;
		//m_librariesEnabled = BoolArray;
		//m_edPointers = EditorPointerArray;
		//m_projectBasePath = "";
		//m_executableName = wxFileName;
	}
}

//////////////////////////////////////////////////////////////////////////////
///  public SetProjectFile
///  Set the project file (blah.cpj).  This will also set the name of the
///     project.
///
///  @param  filename wxFileName  The project file (_.cpj)
///
///  @return void
///
//////////////////////////////////////////////////////////////////////////////
void ProjectInfo::SetProjectFile(wxFileName projfile) {
	m_projectFile = projfile;
	//if(m_projectName == "") {
		SetProjectName(projfile.GetName());
	//}
}

//////////////////////////////////////////////////////////////////////////////
///  public GetProjectBasePath
///  Returns the path of the project file (blah.cpj) with a trailing separator.
///
///  @return wxString   Path of project file as a string, or the path of the
///                        single file.
///
//////////////////////////////////////////////////////////////////////////////
wxString ProjectInfo::GetProjectBasePath() {
	// path is based on file 1, if it's a single file project, or the project
	//    file itself.
	wxFileName f = m_isSingleFile ? wxFileName(m_sourceFiles[0]) : m_projectFile;

	return f.GetPath(wxPATH_GET_VOLUME | wxPATH_GET_SEPARATOR,
								m_isRemote ? wxPATH_UNIX : wxPATH_DOS);
}

//////////////////////////////////////////////////////////////////////////////
///  public GetProjectName
///  This is does not always have to relate to the project's filename.
///
///  @return wxString   The Name of the Project
///
//////////////////////////////////////////////////////////////////////////////
wxString ProjectInfo::GetProjectName() {
	if(m_isSingleFile) {
		wxFileName file(m_sourceFiles[0]);
		return file.GetName();
	}
	else {
		return m_projectName;
	}
}


//////////////////////////////////////////////////////////////////////////////
///  public FileExistsInProject
///  Checks if a filename is currently in the project
///
///  @param  filename wxString  The filename to look for
///
///  @return bool     Whether or not the file is in the project
///
///  @author Mark Erikson @date 04-23-2004
//////////////////////////////////////////////////////////////////////////////
bool ProjectInfo::FileExistsInProject(wxString filename)
{
	bool fileInProject = false;

	if(m_headerFiles.Index(filename) != -1)
	{
		fileInProject = true;
	}
	if(m_sourceFiles.Index(filename) != -1)
	{
		fileInProject = true;
	}
	if(m_libraryFiles.Index(filename) != -1)
	{
		fileInProject = true;
	}
	if(m_nonSourceFiles.Index(filename) != -1)
	{
		fileInProject = true;
	}

	return fileInProject;
}

//////////////////////////////////////////////////////////////////////////////
///  private SelectStringArray
///  An internal utility function which returns a pointer to a designated wxArrayString
///
///  @param  filterType      FileFilterType  Denotes which wxArrayString to return
///
///  @return wxArrayString * The pointer to the selected wxArrayString
///
///  @author Mark Erikson @date 04-23-2004
//////////////////////////////////////////////////////////////////////////////
wxArrayString* ProjectInfo::SelectStringArray(FileFilterType filterType)
{
	switch(filterType)
	{
	case FILE_NSCR:
		return &m_sourceFiles;
	case FILE_NPRC:
		return &m_headerFiles;
	case FILE_DATAFILES:
		return &m_libraryFiles;
	case FILE_NONSOURCE:
		return &m_nonSourceFiles;
	}
	// shouldn't ever get here, just eliminating the "not all code paths
	// return a value" warning
	return NULL;
}

//////////////////////////////////////////////////////////////////////////////
///  private SelectBoolArray
///  An internal utility function which returns a pointer to the designated BoolArray
///
///  @param  filterType  FileFilterType  Denotes which BoolArray to return
///
///  @return BoolArray * A pointer to the selected BoolArray
///
///  @author Mark Erikson @date 04-23-2004
//////////////////////////////////////////////////////////////////////////////
BoolArray* ProjectInfo::SelectBoolArray(FileFilterType filterType)
{
	switch(filterType)
	{
	case FILE_NSCR:
		return &m_sourcesEnabled;
	case FILE_NPRC:
		return &m_headersEnabled;
	case FILE_DATAFILES:
		return &m_librariesEnabled;
	case FILE_NONSOURCE:
		return &m_nonSourcesEnabled;
	}
	// shouldn't ever get here
	return NULL;
}

//////////////////////////////////////////////////////////////////////////////
///  public AddFileToProject
///  Adds a filename to the project
///
///  @param  filename wxString        The name of the file to add
///  @param  fileType FileFilterType  The type of the file to add
///
///  @return void
///
///  @author Mark Erikson @date 04-23-2004
//////////////////////////////////////////////////////////////////////////////
void ProjectInfo::AddFileToProject(wxString filename, FileFilterType filterType)
{
	if(FileExistsInProject(filename)) {
		wxLogDebug("Tried adding a file that's already in the file.");
		return;
	}

	wxArrayString* filelist = SelectStringArray(filterType);
	filelist->Add(filename);
	BoolArray* enablelist = SelectBoolArray(filterType);
	enablelist->Add(true);
/*
	if(filelist->GetCount() > 1)
	{
		SetSingleFile(false);
	}
*/
}

//////////////////////////////////////////////////////////////////////////////
///  public RemoveFileFromProject
///  Removes a file from the project
///
///  @param  filename wxString        The name of the file to remove
///  @param  fileType FileFilterType  The type of the file to remove
///
///  @return void
///
///  @author Mark Erikson @date 04-23-2004
//////////////////////////////////////////////////////////////////////////////
void ProjectInfo::RemoveFileFromProject(wxString filename, FileFilterType filterType)
{
	wxArrayString* filelist = SelectStringArray(filterType);

	int index = filelist->Index(filename);
	filelist->Remove(filename);

	BoolArray* enablelist = SelectBoolArray(filterType);
	enablelist->RemoveAt(index);

/*
	if(filelist->GetCount() < 2)
	{
		SetSingleFile(true);
	}
*/
}

//////////////////////////////////////////////////////////////////////////////
///  public FileIncludedInBuild
///  Checks whether or not a given file is to be included when compiling
///
///  @param  filename   wxString        The name of the file to check
///  @param  filterType FileFilterType  The type of the file to check
///
///  @return bool      Whether or not the file should be included
///
///  @author Mark Erikson @date 04-23-2004
//////////////////////////////////////////////////////////////////////////////
bool ProjectInfo::FileIncludedInBuild(wxString filename, FileFilterType filterType)
{
	wxArrayString* filelist = SelectStringArray(filterType);
	int index = filelist->Index(filename);

	BoolArray* enablelist = SelectBoolArray(filterType);
	return enablelist->Item(index);
}

//////////////////////////////////////////////////////////////////////////////
///  public SetFileBuildInclusion
///  Sets whether or not a given file should be included in compiling
///
///  @param  filename   wxString        The name of the file to set
///  @param  filterType FileFilterType  The type of the file to set
///  @param  enable     bool            Whether or not the file should be included
///
///  @return void
///
///  @author Mark Erikson @date 04-23-2004
//////////////////////////////////////////////////////////////////////////////
void ProjectInfo::SetFileBuildInclusion(wxString filename, FileFilterType filterType, bool enable)
{
	wxArrayString* filelist = SelectStringArray(filterType);
	int index = filelist->Index(filename);

	BoolArray* enablelist = SelectBoolArray(filterType);
	(*enablelist)[index] = enable;
}

//////////////////////////////////////////////////////////////////////////////
///  public GetSourcesToBuild
///  Returns all files that are not excluded from the build
///
///  @return wxArrayString The list of files to be compiled
///
///  @author Mark Erikson @date 04-23-2004
//////////////////////////////////////////////////////////////////////////////
wxArrayString ProjectInfo::GetSourcesToBuild()
{
	wxArrayString sourcesToBuild;

	for(int i = 0; i < (int)m_sourcesEnabled.GetCount(); i++)
	{
		if(m_sourcesEnabled[i])
		{
			sourcesToBuild.Add(m_sourceFiles[i]);
		}
	}

	return sourcesToBuild;
}


//////////////////////////////////////////////////////////////////////////////
///  public AddEditor
///  Adds an editor to the list of open editors for this project
///
///  @param  edit ChameleonEditor * An editor that contains a file from this project
///
///  @return void
///
///  @author Mark Erikson @date 04-23-2004
//////////////////////////////////////////////////////////////////////////////
void ProjectInfo::AddEditor(NumeReEditor* edit)
{
	m_edPointers.Add(edit);
}

//////////////////////////////////////////////////////////////////////////////
///  public RemoveEditor
///  Removes an editor from the list of open editors for this project
///
///  @param  edit ChameleonEditor * An editor that contained a file from this project
///
///  @return void
///
///  @author Mark Erikson @date 04-23-2004
//////////////////////////////////////////////////////////////////////////////
void ProjectInfo::RemoveEditor(NumeReEditor* edit)
{
	m_edPointers.Remove(edit);
}

//////////////////////////////////////////////////////////////////////////////
///  private MakeReadOnly
///  Sets the read-only status of all editors from this project
///
///  @param  makeReadOnly bool  The new read-only status for this project's editors
///
///  @return void
///
///  @author Mark Erikson @date 04-23-2004
//////////////////////////////////////////////////////////////////////////////
void ProjectInfo::MakeReadOnly(bool makeReadOnly)
{
	m_isReadOnly = makeReadOnly;
	for(int i = 0; i < (int)m_edPointers.GetCount(); i++)
	{
		NumeReEditor* ed = m_edPointers[i];
		ed->SetReadOnly(makeReadOnly);
	}
}

//////////////////////////////////////////////////////////////////////////////
///  public SetBeingCompiled
///  Sets the compiling status for this project
///
///  @param  compiling bool  Whether or not this project is being compiled
///
///  @return void
///
///  @author Mark Erikson @date 04-23-2004
//////////////////////////////////////////////////////////////////////////////
void ProjectInfo::SetBeingCompiled(bool compiling)
{
	m_isBeingCompiled = compiling;
	MakeReadOnly(compiling);
}


bool ProjectInfo::IsCompilable()
{
	wxArrayString enabledSources = GetSourcesToBuild();
	return (enabledSources.GetCount() > 0);
}

FileFilterType ProjectInfo::GetFileType(wxString filename)
{
	wxFileName file(filename);

	wxString extension = file.GetExt();

	FileFilterType fileType = FILE_NONSOURCE;

	if(extension == "nprc")
	{
		fileType = FILE_NPRC;
	}
	else if (extension == "nscr")
	{
		fileType = FILE_NSCR;
	}
	else if (extension == "ndat" || extension == "dat" || extension == "csv" || extension == "jdx" || extension == "dx" || extension == "jcm")
	{
		fileType = FILE_DATAFILES;
	}
	else if (extension == "tex")
	{
        fileType = FILE_TEXSOURCE;
	}


	/*

	if(m_headerFiles.Index(filename) != -1)
	{
		fileType = FILE_HEADERS;
	}
	if(m_sourceFiles.Index(filename) != -1)
	{
		fileType = FILE_SOURCES;
	}
	if(m_libraryFiles.Index(filename) != -1)
	{
		fileType = FILE_LIBRARIES;
	}
	if(m_nonSourceFiles.Index(filename) != -1)
	{
		fileType = FILE_NONSOURCE;
	}
	*/

	return fileType;
}

/*
bool ProjectInfo::IsSingleFile()
{
	int numSourceFiles = m_sourceFiles.Count();

	bool isSingleFile = true;

	if(numSourceFiles > 1)
	{
		isSingleFile = false;
	}

	return isSingleFile;
}
*/

wxString ProjectInfo::GetExecutableFileName(bool reverseSlashes)
{
	wxString fullFilePath;

	if(m_isRemote)
	{
		fullFilePath = m_executableName.GetFullPath(wxPATH_UNIX);
	}
	else
	{
		if(!reverseSlashes)
		{
			fullFilePath = m_executableName.GetFullPath(wxPATH_DOS);
		}
		else
		{
			fullFilePath += m_executableName.GetVolume();
			fullFilePath += m_executableName.GetVolumeSeparator();
			fullFilePath += m_executableName.GetPath(wxPATH_GET_VOLUME | wxPATH_GET_SEPARATOR, wxPATH_UNIX);
			fullFilePath += m_executableName.GetFullName();
		}
	}

	return fullFilePath;
}
