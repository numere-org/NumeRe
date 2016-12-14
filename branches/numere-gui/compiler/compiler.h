#ifndef _CHAMEL_COMPILER_H_
#define _CHAMEL_COMPILER_H_

#include <wx/wx.h>
#include "../common/datastructures.h"
class wxEvtHandler;
class Options;
class Networking;
class ProjectInfo;
class ChameleonProcessEvent;
class wxTestOutputStream;


class Compiler : public wxEvtHandler
{
	public:
		Compiler(Options* options, Networking* network);
		~Compiler();
		void CompileProject(ProjectInfo* proj, wxEvtHandler* h);
		bool IsCompiling() { return m_isCompiling || m_isLinking;}
		void HaltCompiling();

	private:
		//Helper:
		void StartNextFile();
		void StartLinking();
		void ParseCompilerMessages(wxString s);
		void RemoveIntermediateFiles();

		wxString CreateLocalCommand(wxString actualCommand);

		//Events:
		void OnProcessTerm(ChameleonProcessEvent& e);
		void OnProcessOut(ChameleonProcessEvent& e);
		void OnProcessErr(ChameleonProcessEvent& e);

		// Data:
		Options* m_options;
		Networking* m_network;
		wxTextOutputStream* m_compilerStdIn; // only used for ^C
		ProjectInfo* m_currProj;
		int m_currFileNum; // currFileIndex might have been a better name
		wxArrayString m_intermediateFiles;
		bool m_isCompiling;
		bool m_isLinking;
		CompileResult m_compilingStatus;

		wxString m_mingwPath;


	DECLARE_EVENT_TABLE()
};

#endif /* _CHAMEL_COMPILER_H_ */
