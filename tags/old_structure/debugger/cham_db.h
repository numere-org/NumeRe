/*****************************************************************\
*Wrapper Header file
*
*author: Ben Carhart
*date started: 1/11/04
*date finished: 4/20/04
*
*description: Code for the implementation of a wrapper for GDB
*revisions: *see cham_db.cpp*
*
\*****************************************************************/

#ifndef CHAMELEON_DEBUGER_H
#define CHAMELEON_DEBUGER_H

#include <wx/string.h>
#include <wx/dynarray.h>
#include <wx/event.h>
#include <wx/txtstrm.h>
#include <wx/regex.h>

#include "../common/chameleonprocessevent.h"
#include "../common/debugevent.h"
#include "../network/networking.h"
#include "../common/datastructures.h"

class ProjectInfo;
class Options;

//global declarations
const wxString PROMPT_CHAR = "`";

//keywords for parsing output
enum DEBUG_KEYS {
	START = -1,				//program start
	CONTINUE = 134242,
	E_BREAK,				//enable break
	D_BREAK,				//disable break
	K_BREAK,				//kill break
	S_BREAK,				//set break
	GO,
	JUMP,
	SOFT_RESET,
	HARD_RESET,
	NEW_FILE,				//new file to debug
	NEW_MODE,				//change in program mode
	NEW_PROC,				//change in process pointer
	VAR_ASSIGN,
	WATCH_VAR,
	STEP,
	STEP_OVER,
	STEP_OUT,
	STOP,
	GET_WHAT,
	GET_PRINT,
	RUN_TO_CURSOR,

	WAITING,	//134260
};

//status of program
enum DEBUG_STATUS {
	DEBUG_RUNNING = 135401,	//program running / command executing
	DEBUG_STOPPED,			//nothing happening right now
	DEBUG_WAIT,				//waiting for instruction
	DEBUG_BREAK,			//program paused (because of breakpoint)
	DEBUG_ERROR,			//problem with debug
	DEBUG_NEED_FILE,		//waiting for a file
	DEBUG_DEAD,				//there AIN'T no debug!
};

class Debugger : public wxEvtHandler
{
	public:
		//defaults
		Debugger(Networking* networking, Options* options, wxEvtHandler* pointer);
		~Debugger();					//destructor

		void onDebugEvent(wxDebugEvent &event);

		//status modifiers
		bool getMode();					//get current mode
		wxString getFile();				//returns current file up for debugging

		int genericStatus();			//returns current debug status
		int programStatus();			//returns EXACT class status

		bool isDebugging();				//returns whether i'm running or not
		bool isPaused();				//returns status == DEBUG_WAIT more or less
		ProjectInfo* getCurrentProject(); // returns the project being debugged

		wxString errorMsg();			//returns error message, if any
		bool resetStatus();				//hard reset * USE CAUTIOUSLY *
		void clearError();				//just resets the status variable

		void setProcess(bool newIsRemote, wxString newFname, wxString nExec);

		//breakpoint management
		void setBreak(wxString srcFile, int lineNum);		//set breakpoint
		void disableBreak(wxString srcFile, int lineNum);	//disable breakpoint
		void enableBreak(wxString srcFile, int lineNum);	//enable a breakpoint
		void killBreak(wxString srcFile, int lineNum);		//delete breakpoint
		int numBreak();										//return the # of breakpoints

		//step management
		void step();
		void stepOver();				//execute function lines (don't go into)
		void stepOut();					//execute current stack frame to finish
		void go();						//run program
		void cont();					//continue debugging
		void runToCursor(wxString srcFile, int lineNum);

		//variable management
		void snoopVar(wxString varName, wxString funcName, wxString className, bool oneShot = true);
		void setVar(wxString varName, wxString newValue, wxString funcName, wxString className);
		void removeVar(wxString varName, wxString funcName, wxString className);

		//misc management
		void stop(bool pleaseRestart);	//kills process & reloads it (?)
		void kill();					//simple?
		void sendCustomCommand(wxString cust);	//send a custom command to the process
		wxString getProcOutput(int numEntries);	//get the process's output to X entries back

		void SetEventHandler(wxEvtHandler* handler) { m_parentEventHandler = handler; }

	private:
		void startProcess(bool fullRestart, bool mode, wxString fName, wxString execThis);

		//misc functions i thought would be handy
		void updateHistory(wxString addMe);	//so I don't have to do it manualy
		wxString getHistoryItem(int offset);//ditto
		void flushBuffer();					//flushes the input-data array
		void flushPrivateVar();				//flushes all private variables

		void sendWhat();					//for use with snoopVar
		void sendPrint(wxString fromGDB);	//for use with snoopVar as well

		bool ParseVariableTypes(wxString& fromGDB);
		bool parsePrintOutput(wxString fromGDB, wxArrayString &varValue);
		int findBreakpoint(wxString fName, int lineNum, bool andRemove = false);
		bool checkOutputStream(wxString stream);	//true = okay to parse further

		void sendCommand(wxString send);	//sends command & updates history

		void makeGenericError(wxString comment);
		void addErrorHist(wxString comment);//updates error history

		//ASYNC Event Triggered Functions
		void onProcessOutputEvent(ChameleonProcessEvent &e);
		void onProcessErrOutEvent(ChameleonProcessEvent &e);
		void onProcessTermEvent(ChameleonProcessEvent &e);

		//private elements [grouped according to function]
		bool m_isRemote;				//stores current mode: false = local
		bool m_procLives;				//stores status of async process
		bool m_fileIsSet;				//stores whether a file is loaded
		wxString m_currFile;			//stores current filename for debugging

		int m_status;					//status of debugger
		int m_classStatus;			//holds the last function called
		long m_pid;					//process ID of GDB (if running)

		int m_gdbBreakpointNum;		//keeps track of GDB's #ing system
		int m_numBreakpoints;			//keeps track of actual number of live ones
		DebugBreakHash m_lineToNum;

		wxArrayString m_commandHistory;	//stores command history
		int m_histCount;				//points to current top command

		wxString m_error;				//holds error messages
		wxArrayString errorHist;	//holds ALL error messages ^_^
		int m_errorCount;				//counts the errors

		wxString m_command;			//holds current / last GDB command
		wxString m_returnChar;		//contains the \n or \r for each command
		wxArrayString m_data;			//holds data from GDB
		wxArrayString m_fullOutput;	//holds ALL data back from the process

		//globals for use in parse
		wxString m_Filename;
		wxString m_Linenumber;
		wxString m_FuncName;

		int m_varCount;				//holds array position to insert next var

		VariableInfoArray m_varInfo;//same as VariableInfoHash... only an array
		VariableInfoArray m_oneShotWatches;

		wxString m_firstExecString;	//holds the first executed string
		wxString m_currentSourceName;	//holds filename source

		Networking *m_myConnection;	//the networking object for remote
		Options* m_options;
		wxTextOutputStream* m_streamOut;	//output to GDB
		wxDebugEvent* m_myEvent;		//how i communicate

		//GUI connectivity
		wxEvtHandler* m_parentEventHandler;
		ProjectInfo* m_projectBeingDebugged;

		int m_numberOfDebugEvents;

		DECLARE_EVENT_TABLE()
};

#endif
