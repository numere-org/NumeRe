/*****************************************************************\
*Wrapper Code file
*
*author: Ben Carhart
*date started: 1/11/04
*date finished: 4/20/04
*
*description: Code for the implementation of a wrapper for GDB
*revisions:
*BSC-02/13/04-> Removed all constructors & added 4 private func.
*               to aid in general stuff.
*BSC-02/27/04-> Added Dave Czechowski's ASYNC code which rendered
*               much of my code obsolete.  Backed up files &
*               deleted all commented code to start over.  :(
*BSC-02/29/04-> Added carriage returns [\r] on all my commands. I
*               also added a soft-error-reset as well as a flush-
*               buffer function to clear out unwanted input.
*BSC-03/9/04-> Gosh I added so much i can't list it here.  I added
*              a class-specific status variable, new arrays, a few
*              new functions... re-did a few functions, changed
*              some logic.  I feel like I'm debugging without
*              compiling!  AUGH!
*BSC-03/19/04->Alright... i'm adapting my code into Dave's window
*              API so I can test it.  All portions with "cout"
*              debug statements have been altered & "dave's code"
*              shows obvious places i've moded.
*BSC-04/03/04->So much has changed that it's not funny.  Debug
*              event code has been changed to work with projects.
*BSC-04/20/04->I somehow broke my code in the last update.  My
*              comment here was lost as well.  So I can say we've
*              updated networking and added run-to-cursor ability.
*              Also user I/O is in here (sorta).  Hopefully this
*              version will run...
*BSC-04/23/04->Doxygen Update.
\*****************************************************************/


#include "cham_db.h"
#include "../common/ProjectInfo.h"
#include "../common/Options.h"
#include <wx/wfstream.h>
#include <wx/RegEx.h>
#include <wx/tokenzr.h>

#include <wx/arrimpl.cpp> // this is a magic incantation which must be done!
WX_DEFINE_OBJARRAY(VariableInfoArray);


#include "../common/debug.h"

#ifdef _DEBUG
#define new DEBUG_NEW
#endif

//event handling
BEGIN_EVENT_TABLE(Debugger, wxEvtHandler)
	EVT_PROCESS_ENDED(Debugger::onProcessTermEvent)
	EVT_PROCESS_STDOUT(Debugger::onProcessOutputEvent)
	EVT_PROCESS_STDERR(Debugger::onProcessErrOutEvent)

	EVT_DEBUG(Debugger::onDebugEvent)
END_EVENT_TABLE()

//////////////////////////////////////////////////////////////////////////////
///  public constructor Debugger
///
///  @param  networking Networking *   <contains connection to GDB>
///  @param  pointer    wxEvtHandler * <how i react to GDB (events)>
///
///  @return void
///
///  @author Ben Carhart @date 04-23-2004
//////////////////////////////////////////////////////////////////////////////
Debugger::Debugger(Networking* networking, Options* options, wxEvtHandler* pointer)
{
	flushPrivateVar();
	m_varCount = 0;
	m_numberOfDebugEvents = 0;

	m_myEvent = NULL;

	m_parentEventHandler = pointer;
	m_procLives = false;
	m_status = DEBUG_DEAD;
	m_classStatus = STOP;
	m_myConnection = networking;
	m_options = options;
}
//end constructors

//////////////////////////////////////////////////////////////////////////////
///  public destructor ~Debugger
///
///  @return void
///
///  @author Ben Carhart @date 04-23-2004
//////////////////////////////////////////////////////////////////////////////
Debugger::~Debugger()
{
	//initial stuff to cleanly exit (sorta)
	stop(false);

	if(m_myEvent != NULL)
	{
		delete m_myEvent;
	}

	flushPrivateVar();
	m_varInfo.Clear();
}//end ~Debugger

//////////////////////////////////////////////////////////////////////////////
///  public onDebugEvent
///  <Parses an event raised from the GUI (button push or other)>
///
///  @param  event wxDebugEvent & <the event information from the GUI>
///
///  @return void
///
///  @author Ben Carhart @date 04-23-2004
//////////////////////////////////////////////////////////////////////////////
void Debugger::onDebugEvent(wxDebugEvent &event)
{
	int eventCommand = event.GetId();

	/* We can only accept certain types of events while stopped/paused.
	 * If we're running and we get an unacceptable event type for the current
	 * state, we put it back on the event queue.  We use a counter with a
	 * completely arbitrary number (say, 50) to keep us from going into an
	 * infinite loop.  If we've received events 50 times without actually
	 * processing one, then we'll just pitch it and return.  This combination
	 * prevents stuff like getting stuck after repeatedly pressing Step too fast.
	 */
	if(eventCommand != ID_DEBUG_STOP &&
		eventCommand != ID_DEBUG_START &&
		eventCommand != ID_DEBUG_ADD_WATCH &&
		eventCommand != ID_DEBUG_REMOVE_WATCH)
	{
		if(m_status != DEBUG_WAIT &&
			m_status != DEBUG_BREAK)
		{
			if(m_numberOfDebugEvents < 50)
			{
				this->AddPendingEvent(event);
			}

			m_numberOfDebugEvents++;
			return;
		}
	}
	m_numberOfDebugEvents = 0;

	if(m_myEvent != NULL)
	{
		delete m_myEvent;
	}
	m_myEvent = (wxDebugEvent*)event.Clone();
	//all the variables i need here:

	int eventLineNum;

	wxArrayString gui_var_names;
	wxString varName, funcName, className;

	wxArrayInt breakpointLines;

	FileBreakpointHash breakpointList;

	wxString tmp;
	wxString firstFile;
	wxString sendAllBreakpoints;

	if( eventCommand == ID_DEBUG_ADD_BREAKPOINT ||
		eventCommand == ID_DEBUG_REMOVE_BREAKPOINT ||
		eventCommand == ID_DEBUG_RUNTOCURSOR)
	{
		firstFile = event.GetSourceFilename();
		eventLineNum = event.GetLineNumber();
	}

	if(eventCommand == ID_DEBUG_START)
	{
		breakpointList = event.GetFileBreakpoints();
	}

	if( eventCommand == ID_DEBUG_ADD_WATCH
		|| eventCommand == ID_DEBUG_REMOVE_WATCH
		|| eventCommand == ID_DEBUG_DISPLAY_SELECTION)
	{
		gui_var_names = event.GetVariableNames();
		varName = gui_var_names[0];
		funcName = event.GetFunctionName();
		className = event.GetClassName();
	}

	//loop vars
	int i = 0, j = 0;

	switch(eventCommand)
	{
	case ID_DEBUG_START:
	{

		//start process
		ProjectInfo* proj = event.GetProject();

		wxArrayString srcFiles = proj->GetSources();
		wxString execFile = proj->GetExecutableFileName(true);
		m_projectBeingDebugged = proj;

		startProcess(true, event.IsRemote(), execFile, "gdb -q");

		//set breakpoints
		for(i = 0; i < (int)srcFiles.GetCount(); i++)
		{
			tmp = srcFiles[i];

			wxString filename = tmp;

			if(!proj->IsRemote())
			{
				filename.Replace("\\", "/");
			}

			if(breakpointList.find(tmp) != breakpointList.end())
			{
				breakpointLines = breakpointList[tmp];

				for(j = 0; j < (int)breakpointLines.GetCount(); j++)
				{
					sendAllBreakpoints<<"break \""<<filename<<":"<<breakpointLines[j]<<"\""<<m_returnChar;

					m_lineToNum[tmp].lineNumbers.Add(breakpointLines[j]);
					m_lineToNum[tmp].gdbNumbers.Add(m_gdbBreakpointNum);

					m_numBreakpoints++;
					m_gdbBreakpointNum++;

					//~~DEBUG CODE~~//
					//wxLogDebug("--STARTUP: make breakpoint: num=%d, gdb_num=%d--",numBreakpoints, gdbBreakpointNum);
				}
			}//end if
		}

		//~~DEBUG CODE~~//
		//wxLogDebug("--STARTUP: breakpoint string:\n------\n" + sendAllBreakpoints + "\n\n");


		//sendCommand("list"+m_returnChar);
		sendCommand(sendAllBreakpoints);
		sendCommand("echo Initialization done"+m_returnChar);	//tag... not really a command
		break;
	}

	case ID_DEBUG_STOP:
	{
		stop(false);
		wxDebugEvent dbg;
		dbg.SetId(ID_DEBUG_EXIT_NORMAL);
		m_parentEventHandler->AddPendingEvent(dbg);
		break;
	}

	case ID_DEBUG_STEPNEXT:
		step();
		break;

	case ID_DEBUG_STEPOVER:
		stepOver();
		break;

	case ID_DEBUG_STEPOUT:
		stepOut();
		break;
	case ID_DEBUG_CONTINUE:
		cont();
		break;

	case ID_DEBUG_RUNTOCURSOR:
		runToCursor(firstFile, eventLineNum);
		break;

	case ID_DEBUG_ADD_BREAKPOINT:
		setBreak(firstFile, eventLineNum);
		break;

	case ID_DEBUG_REMOVE_BREAKPOINT:
		killBreak(firstFile, eventLineNum);
		break;

	case ID_DEBUG_ADD_WATCH:
		snoopVar(varName, funcName, className, false);
		break;

	case ID_DEBUG_REMOVE_WATCH:
		removeVar(varName, funcName, className);
		break;

	default:
		tmp = m_error;
		eventLineNum = m_status;
		m_error = "Unrecognized Debug Event";
		makeGenericError("onDebugEvent: ");
		m_error = tmp;
		m_status = eventLineNum;
		break;
	}
}

//PUBLIC FUNCTIONS:
//-----------------
//PROGRAM STATUS
//--------------
//This is just general running information I thought would be
//useful to know while using the class.

//////////////////////////////////////////////////////////////////////////////
///  public getMode
///  <returns current mode (local = false)>
///
///  @return bool <current mode>
///
///  @author Ben Carhart @date 04-23-2004
//////////////////////////////////////////////////////////////////////////////
bool Debugger::getMode()
{
	return(m_isRemote);
}

//////////////////////////////////////////////////////////////////////////////
///  public isDebugging
///  <returns an answer to the generic question, is this class active>
///
///  @return bool <true if process is active & is running or waiting>
///
///  @author Ben Carhart @date 04-23-2004
//////////////////////////////////////////////////////////////////////////////
bool Debugger::isDebugging()
{
	return((m_status == DEBUG_RUNNING) || (m_status == DEBUG_WAIT) || (m_status == DEBUG_BREAK));
}

//////////////////////////////////////////////////////////////////////////////
///  public isPaused
///  <returns an answer to the generic question, can i tell this class something>
///
///  @return bool <true if process is waiting>
///
///  @author Ben Carhart @date 04-23-2004
//////////////////////////////////////////////////////////////////////////////
bool Debugger::isPaused()
{
	return(m_status == DEBUG_WAIT || m_status == DEBUG_BREAK);
}

//////////////////////////////////////////////////////////////////////////////
///  public getCurrentProject
///  <for the GUI>
///
///  @return ProjectInfo * <current C++ project being run>
///
///  @author Ben Carhart @date 04-23-2004
//////////////////////////////////////////////////////////////////////////////
ProjectInfo* Debugger::getCurrentProject()
{
	return m_projectBeingDebugged;
}
//--//

//////////////////////////////////////////////////////////////////////////////
///  public getFile
///  <not sure if it's ever used>
///
///  @return wxString <current file assigned to this class>
///
///  @author Ben Carhart @date 04-23-2004
//////////////////////////////////////////////////////////////////////////////
wxString Debugger::getFile()
{
	return(m_currFile);
}

//////////////////////////////////////////////////////////////////////////////
///  public programStatus
///  <the specific function-by-function class status>
///
///  @return int <the enum declared in the header file>
///
///  @author Ben Carhart @date 04-23-2004
//////////////////////////////////////////////////////////////////////////////
int Debugger::programStatus()
{
	return(m_classStatus);
}

//genericStatus(): returns the generic debugger status
//////////////////////////////////////////////////////////////////////////////
///  public genericStatus
///  <the generic class status>
///
///  @return int <the enum declared in the header file>
///
///  @author Ben Carhart @date 04-23-2004
//////////////////////////////////////////////////////////////////////////////
int Debugger::genericStatus()
{
	return(m_status);
}

//////////////////////////////////////////////////////////////////////////////
///  public errorMsg
///  <a generic way of seeing the last error; also prob. never used>
///
///  @return wxString <contents of the last error>
///
///  @author Ben Carhart @date 04-23-2004
//////////////////////////////////////////////////////////////////////////////
wxString Debugger::errorMsg()
{
	return(m_error);
}

//////////////////////////////////////////////////////////////////////////////
///  public resetStatus
///  <an attempt to reset everything as best this class can>
///
///  @return bool <false if there is no process to reset>
///
///  @author Ben Carhart @date 04-23-2004
//////////////////////////////////////////////////////////////////////////////
bool Debugger::resetStatus()
{
	if(m_status == DEBUG_RUNNING)
	{
		//if it's running, kill it...
		stop(true);

		m_status = DEBUG_STOPPED;
	}

	//i figure that if someone wants to reset the Debugger, then
	//darn it EVERYTHING is gonna get reset. so....
	//do a full reset of GDB

	if(m_status != DEBUG_DEAD)
	{
		wxArrayString debugReset;

		if(numBreak() > 0)
		{
			debugReset.Add("delete breakpoint" + m_returnChar);
			m_gdbBreakpointNum = 0;
			m_numBreakpoints = 0;
			m_lineToNum.empty();
		}

		//if the file was just loaded... no need to kill something not
		//running.  ^_^  + it generates an error
		if(m_fileIsSet)
		{
			if(m_status != DEBUG_STOPPED)
			{
				debugReset.Add("kill" + m_returnChar);
			}

			debugReset.Add("file" + m_returnChar);
			debugReset.Add("file " + m_currFile + m_returnChar);

			//now change the status
			m_status = DEBUG_STOPPED;
		}

		//end reset commands...
		m_classStatus = HARD_RESET;

		for(int c = 0; c < int(debugReset.GetCount()); c++)
		{
			sendCommand(debugReset[c]);
		}

		m_error = "";
		m_command = "";
		m_data.Empty();

		return(true);
	}

	//this means the proc is dead so there's nothing to reset
	return(false);
}

//////////////////////////////////////////////////////////////////////////////
///  public clearError
///  <a softer version of [resetStatus()]; just re-sets the status vars>
///
///  @return void
///
///  @author Ben Carhart @date 04-23-2004
//////////////////////////////////////////////////////////////////////////////
void Debugger::clearError()
{
	if(m_fileIsSet)
	{
		if(m_classStatus == START || m_classStatus == HARD_RESET)
		{
			m_status = DEBUG_STOPPED;
		}
		else
		{
			m_status = DEBUG_WAIT;
		}
	}
	else
	{
		m_status = DEBUG_NEED_FILE;
	}

	m_classStatus = SOFT_RESET;
}

//////////////////////////////////////////////////////////////////////////////
///  public setProcess
///  <changes / redirects the process pointer; probably never used>
///
///  @param  newIsRemote bool      <mode of the new process>
///  @param  newFname    wxString  <new file name>
///  @param  nExec       wxString  <new executable command>
///
///  @return void
///
///  @author Ben Carhart @date 04-23-2004
//////////////////////////////////////////////////////////////////////////////
void Debugger::setProcess(bool newIsRemote, wxString newFname, wxString nExec)
{
	//begin with a hard reset of everything.
	stop(false);
	flushPrivateVar();

	startProcess(true,newIsRemote, newFname, nExec);
}


//////////////////////////////////////////////////////////////////////////////
///  private startProcess
///  <begins the process; status=DEBUG_DEAD till this point>
///
///  @param  fullRestart bool      <if we just want to start the process
///                                 again, don't reset all the internal vars>
///  @param  mode        bool      <process mode>
///  @param  fName       wxString  <filename to load>
///  @param  execThis    wxString  <the command to execute>
///
///  @return void
///
///  @author Ben Carhart @date 04-23-2004
//////////////////////////////////////////////////////////////////////////////
void Debugger::startProcess(bool fullRestart, bool mode, wxString fName, wxString execThis)
{
	if(fullRestart)
	{

		//generic initialization
		m_isRemote = mode;			//false = local
		m_currFile = fName;

		m_histCount = 0;

		m_numBreakpoints = 0;
		m_gdbBreakpointNum = 1;

		m_errorCount = 0;

		m_firstExecString = execThis;
	}

	if(m_isRemote)
	{
		m_returnChar = "\r";
	}
	else
	{
		m_returnChar = "\r\n";

		StringFilenameHash mingwExecutables = m_options->GetMingwExecutables();
		wxFileName gdbPath = mingwExecutables["gdb.exe"];

		wxString localGDBCommand;
		localGDBCommand.Printf("\"%s\" -q", gdbPath.GetFullPath());
		//execThis.Replace("gdb", gdbPath.GetFullPath());
		execThis = localGDBCommand;
	}

	if(fName != "")
	{
		m_status = DEBUG_STOPPED;
		m_fileIsSet = true;
	}
	else
	{
		m_status = DEBUG_NEED_FILE;
		m_fileIsSet = false;
	}

	m_procLives = true;
	m_classStatus = NEW_PROC;


	m_streamOut = m_myConnection->StartCommand(m_isRemote, execThis, this);

	m_command = execThis;
	updateHistory(m_command);

	//initial commands to GDB

	wxArrayString initString;
	wxString tmp;

	m_classStatus = START;

	//initialize GDB
	tmp.Printf("set prompt %s%s", PROMPT_CHAR.c_str(), m_returnChar.c_str());
	initString.Add(tmp);
	initString.Add("set print array off" + m_returnChar);
	initString.Add("set print pretty off" + m_returnChar);
	initString.Add("set print address off" + m_returnChar);
	initString.Add("set confirm off" + m_returnChar);
	initString.Add("set print repeats 10000" + m_returnChar);

	if(m_isRemote)
	{
		wxString ttyString = m_myEvent->GetTTYString();
		initString.Add("tty " + ttyString + m_returnChar);
	}

	//send all initlization commands to GDB
	for(int currInit = 0; currInit < int(initString.GetCount()); currInit++)
	{
		sendCommand(initString.Item(currInit));
	}

	if(m_fileIsSet)
	{
		m_command = "file " + m_currFile + m_returnChar;
		sendCommand(m_command);
	}//end set-file conditional
}
//END PROGRAM STATUS

//BREAKPOINT FUNCTIONALITY
//------------------------
//i used to have 3 variables to use for breakpoints.
//Now I don't know.  ^_^  We re-designed things.

//////////////////////////////////////////////////////////////////////////////
///  public setBreak
///  <sets a breakpoint [GDB SPECIFIC]>
///
///  @param  srcFile wxString  <the file to set the breakpoint in>
///  @param  lineNum int       <the linenumber.  This and the above item are
///                             parsed into ["file:linenum"]>
///
///  @return void
///
///  @author Ben Carhart @date 04-23-2004
//////////////////////////////////////////////////////////////////////////////
void Debugger::setBreak(wxString srcFile, int lineNum)
{
	if(m_fileIsSet &&
		(m_status == DEBUG_WAIT || m_status == DEBUG_BREAK))
	{

		//this if{} should never be run...
		if(numBreak() == 0 && m_gdbBreakpointNum == 1)
		{
			m_classStatus = START;	//ignore the output

			m_gdbBreakpointNum = 1;
		}

		m_classStatus = S_BREAK;
		m_command.Printf("break \"%s:%d\"%s", srcFile, lineNum, m_returnChar.c_str());
		sendCommand(m_command);

		//~~DEBUG CODE~~//
		//wxLogDebug("--make breakpoint:(+1 to follow) num=%d, gdb_num=%d--",numBreakpoints, gdbBreakpointNum);

		m_lineToNum[srcFile].lineNumbers.Add(lineNum);
		m_lineToNum[srcFile].gdbNumbers.Add(m_gdbBreakpointNum);
		m_numBreakpoints++;
		m_gdbBreakpointNum++;
	}
}

//////////////////////////////////////////////////////////////////////////////
///  public enableBreak
///  <enables a disabled breakpoint (or not) [GDB SPECIFIC]>
///
///  @param  srcFile wxString  <file with the breakpoint>
///  @param  lineNum int       <complementary line number>
///
///  @return void
///
///  @author Ben Carhart @date 04-23-2004
//////////////////////////////////////////////////////////////////////////////
void Debugger::enableBreak(wxString srcFile, int lineNum)
{
	int equivNum = findBreakpoint(srcFile, lineNum);

	if(equivNum == -1)
	{
		wxString baka = m_error;
		int suBaka = m_status;

		m_error.Printf("Failed to locate breakpoint in %s at %d", srcFile.c_str(), lineNum);
		makeGenericError("enableBreak: ");

		m_error = baka;
		m_status = suBaka;
	}
	else
	{
		m_command.Printf("enable %d%s", equivNum, m_returnChar.c_str());
		sendCommand(m_command);

		m_classStatus = E_BREAK;
	}
}

//////////////////////////////////////////////////////////////////////////////
///  public disableBreak
///  <disables a breakpoint in a given file at a given line>
///
///  @param  srcFile wxString  <the file>
///  @param  lineNum int       <the complementary line #>
///
///  @return void
///
///  @author Ben Carhart @date 04-23-2004
//////////////////////////////////////////////////////////////////////////////
void Debugger::disableBreak(wxString srcFile, int lineNum)
{
	int equivNum = findBreakpoint(srcFile, lineNum);

	if(equivNum == -1)
	{
		wxString baka = m_error;
		int suBaka = m_status;

		m_error.Printf("Failed to locate breakpoint in %s at %d", srcFile.c_str(), lineNum);
		makeGenericError("disableBreak: ");

		m_error = baka;
		m_status = suBaka;
	}
	else
	{
		m_command.Printf("disable %d%s", equivNum, m_returnChar.c_str());
		sendCommand(m_command);

		m_classStatus = D_BREAK;
	}
}

//////////////////////////////////////////////////////////////////////////////
///  public killBreak
///  <deletes a breakpoint>
///
///  @param  srcFile wxString  <file>
///  @param  lineNum int       <linenumber to kill>
///
///  @return void
///
///  @author Ben Carhart @date 04-23-2004
//////////////////////////////////////////////////////////////////////////////
void Debugger::killBreak(wxString srcFile, int lineNum)
{
	int equivNum = findBreakpoint(srcFile, lineNum, true);
	if(equivNum == -1)
	{
		wxString baka = m_error;
		int suBaka = m_status;

		m_error.Printf("Failed to locate breakpoint in %s at %d", srcFile.c_str(), lineNum);
		makeGenericError("killBreak: ");

		m_error = baka;
		m_status = suBaka;
	}

	m_classStatus = K_BREAK;
}

//////////////////////////////////////////////////////////////////////////////
///  private findBreakpoint
///  <a private function to automate the above 3 functions>
///
///  @param  fName     wxString  <filename to look for>
///  @param  lineNum   int       <line-num to test against the file>
///  @param  andRemove bool      [=false] <facilitates [killBreak()]>
///
///  @return int       <returns a [GDB SPECIFIC] number associated with every
///                     breakpoint created>
///
///  @author Ben Carhart @date 04-23-2004
//////////////////////////////////////////////////////////////////////////////
int Debugger::findBreakpoint(wxString fName, int lineNum, bool andRemove)
{
	//altered... added wxArrayInt & took (lineToNum[fName].lineNumbers) out
	bool found = false;
	int equivNum = 0;
	int arrayCount = m_lineToNum[fName].lineNumbers.GetCount();
	wxArrayInt tmp = m_lineToNum[fName].lineNumbers;

	int lineIndex = tmp.Index(lineNum);
	if(lineIndex != wxNOT_FOUND)
	{
		if(andRemove)
		{
			//~~DEBUG CODE~~//
			//wxLogDebug("--find breakpoint: i=%d, equivNum=%d, ltn[fn].gn.remove(i-1): %d--", i, equivNum, lineToNum[fName].gdbNumbers[i - 1]);

			int equivNum = m_lineToNum[fName].gdbNumbers[lineIndex];

			//~~DEBUG CODE~~//
			//wxLogDebug("--find breakpoint: index=%d, equivNum=%d--", lineIndex, equivNum);

			m_lineToNum[fName].lineNumbers.Remove(lineNum);
			m_lineToNum[fName].gdbNumbers.Remove(equivNum);

			m_command.Printf("delete break %d%s", equivNum, m_returnChar.c_str());
			sendCommand(m_command);

			m_numBreakpoints--;
		}

		return(equivNum);
	}
	return(-1);
}

//////////////////////////////////////////////////////////////////////////////
///  public numBreak
///  <accessor function for the # of current breakpoints set>
///
///  @return int <# of current breakpoints set>
///
///  @author Ben Carhart @date 04-23-2004
//////////////////////////////////////////////////////////////////////////////
int Debugger::numBreak()
{
	return(m_numBreakpoints);
}
//END BREAKPOINT SECTION

//STEP FUNCTIONALITY
//------------------
//This section handles step management; the jump-over, step into, things
//like that.  The "go" function is also here.  It will get private var's
//as neccesary.

//////////////////////////////////////////////////////////////////////////////
///  public step
///  <will step the program a literal one line [GDB SPECIFIC]>
///
///  @return void
///
///  @author Ben Carhart @date 04-23-2004
//////////////////////////////////////////////////////////////////////////////
void Debugger::step()
{

	//check program status
	if(m_status == DEBUG_STOPPED)
	{
		m_command = "run" + m_returnChar;
		m_status = DEBUG_RUNNING;
		m_classStatus = GO;
		sendCommand(m_command);

	}
	else if(m_status == DEBUG_WAIT || m_status == DEBUG_BREAK)
	{
		m_command = "step" + m_returnChar;
		m_status = DEBUG_RUNNING;
		m_classStatus = STEP;
		sendCommand(m_command);
	}
	//if not stopped or waiting, then program status = error
}

//////////////////////////////////////////////////////////////////////////////
///  public stepOver
///  <nearly identical to step, only steps one line in present function; aka
///   skips over function calls [GDB SPECIFIC]>
///
///  @return void
///
///  @author Ben Carhart @date 04-23-2004
//////////////////////////////////////////////////////////////////////////////
void Debugger::stepOver()
{
	//check program status
	if(m_status == DEBUG_WAIT || m_status == DEBUG_BREAK)
	{
		m_command = "next" + m_returnChar;
		m_status = DEBUG_RUNNING;
		m_classStatus = STEP_OVER;
		sendCommand(m_command);
	}
	//if not waiting, then program status is not good for a step-over
}

//////////////////////////////////////////////////////////////////////////////
///  public stepOut
///  <finishes the current function & stops when stack is reached 1 level up
///   [GDB SPECIFIC]>
///
///  @return void
///
///  @author Ben Carhart @date 04-23-2004
//////////////////////////////////////////////////////////////////////////////
void Debugger::stepOut()
{
	//check program status
	if(m_status == DEBUG_WAIT || m_status == DEBUG_BREAK)
	{
		m_command = "finish" + m_returnChar;
		m_status = DEBUG_RUNNING;
		m_classStatus = STEP_OUT;
		sendCommand(m_command);
	}

	//if not stopped or waiting, then program status is not good for stepOut
}

//////////////////////////////////////////////////////////////////////////////
///  public runToCursor
///  <runs from current place in debugged program to where the cursor is [GDB
///   SPECIFIC]; related to breakpoints>
///
///  @param  srcFile wxString  <file to run-to in>
///  @param  lineNum int       <line to run-to>
///
///  @return void
///
///  @author Ben Carhart @date 04-23-2004
//////////////////////////////////////////////////////////////////////////////
void Debugger::runToCursor(wxString srcFile, int lineNum)
{
	if(m_status == DEBUG_WAIT || m_status == DEBUG_BREAK)
	{
		//GDB automatically sets & kills the breakpoint, but the number is incremented
		m_gdbBreakpointNum++;

		m_command.Printf("tbreak \"%s:%d\"%s", srcFile, lineNum, m_returnChar);
		sendCommand(m_command);

		cont();

		m_classStatus = RUN_TO_CURSOR;
	}

	//if not stopped or waiting, then program status is not good for stepOut
}

//////////////////////////////////////////////////////////////////////////////
///  public go
///  <sends a [GDB SPECIFIC] "go" command>
///
///  @return void
///
///  @author Ben Carhart @date 04-23-2004
//////////////////////////////////////////////////////////////////////////////
void Debugger::go()
{
	//check to make sure the program is suitable to BE run...
	if(m_status != DEBUG_ERROR ||
		m_status != DEBUG_RUNNING ||
		m_status != DEBUG_NEED_FILE ||
		m_status != DEBUG_DEAD)
	{
		m_classStatus = GO;
		m_status = DEBUG_RUNNING;

		m_command = "run" + m_returnChar;
		sendCommand(m_command);
	}//end status check
}//wow, tough

//////////////////////////////////////////////////////////////////////////////
///  public stop
///  <attempts a clean cease-and-decist of the process [GDB SPECIFIC]>
///
///  @param  pleaseRestart bool  <use this if you want absolute full-restart>
///
///  @return void
///
///  @author Ben Carhart @date 04-23-2004
//////////////////////////////////////////////////////////////////////////////
void Debugger::stop(bool pleaseRestart)
{
	int statBackup = m_status;
	int classStatB = m_classStatus;

	if(m_status == DEBUG_RUNNING || m_status == DEBUG_ERROR)
	{
		m_myConnection->ForceKillProcess(m_streamOut);
	}
	else if(m_status == DEBUG_DEAD)
	{
		m_error = "tried to stop dead process";
		makeGenericError("stop(): ");
		m_status = DEBUG_DEAD;
	}
	else
	{
		//try to stop as cleanly as possible?
		if(numBreak() > 0)
		{
			m_command = "delete breakpoint" + m_returnChar;
			sendCommand(m_command);
		}

		if(m_varCount > 0)
		{
			m_command = "delete display" + m_returnChar;
			sendCommand(m_command);
		}

		if(m_fileIsSet)
		{
			m_command = "file" + m_returnChar;
			sendCommand(m_command);
		}

		m_command = "quit" + m_returnChar;
		sendCommand(m_command);

	}

	//is this appropriate for me???
	if(m_isRemote)
	{
		if(statBackup != DEBUG_RUNNING &&
			statBackup != DEBUG_ERROR &&
			statBackup != DEBUG_DEAD)
		{
			m_command = "exit" + m_returnChar;
			sendCommand(m_command);
		}
	}

	flushPrivateVar();

	m_classStatus = STOP;
	m_status = DEBUG_DEAD;
	resetStatus();

	m_procLives = false;

	if(pleaseRestart)
	{
		startProcess(true, m_isRemote, m_currFile, m_firstExecString);
	}//end pleaseRestart conditional
}

//////////////////////////////////////////////////////////////////////////////
///  public kill
///  <questions?  e-mail lyrisriver@hotmail.com so i can laugh at you>
///
///  @return void
///
///  @author Ben Carhart @date 04-23-2004
//////////////////////////////////////////////////////////////////////////////
void Debugger::kill()
{
	m_classStatus = STOP;
	m_status = DEBUG_DEAD;

	m_procLives = false;

	m_myConnection->ForceKillProcess(m_streamOut);

	flushBuffer();
	flushPrivateVar();
}

//////////////////////////////////////////////////////////////////////////////
///  public cont
///  <tells a stopped or waiting program to "go" again [GDB SPECIFIC]>
///
///  @return void
///
///  @author Ben Carhart @date 04-23-2004
//////////////////////////////////////////////////////////////////////////////
void Debugger::cont()
{
	//continue will only work correctly on these two status items
	if(m_status == DEBUG_WAIT || m_status == DEBUG_BREAK)
	{
		m_command = "continue" + m_returnChar;
		m_classStatus = CONTINUE;
		m_status = DEBUG_RUNNING;
		sendCommand(m_command);
	}
}
//END STEP FUNCTIONALITY

//VARIABLE FUNCTIONALITY
//----------------------

//////////////////////////////////////////////////////////////////////////////
///  public snoopVar
///  <the "watchAVariable" function>
///
///  @param  varName   wxString  <the variable name to watch>
///  @param  funcName  wxString  <-not used- where in the current prog it is>
///  @param  className wxString  <-not used- a specific class?>
///  @param  oneShot   bool      [=true] <relic code not used at all>
///
///  @return void
///
///  @author Ben Carhart @date 04-23-2004
//////////////////////////////////////////////////////////////////////////////
void Debugger::snoopVar(wxString varName, wxString funcName, wxString className, bool oneShot)
{
	wxString combinedFunction;
	VariableInfo variableStruct;
	bool notFound = true;

	combinedFunction<<className<<"::"<<funcName;

	//Step 1: See if the variable & function/class exists in the structure array
	if(m_varCount == 0)
	{
		//we're adding the first item to the array
		variableStruct.name = varName;
		variableStruct.functionName = combinedFunction;
		variableStruct.type = "null";
		variableStruct.value = "0";

		m_varInfo.Add(variableStruct);

		m_varCount++;
	}
	else
	{
		//check to see if the variable is already in the array
		for(int i = 0; (i < m_varCount) && (notFound); i++)
		{
			if(m_varInfo[i].name == varName)
			{
				notFound = false;
			}
		}

		if(notFound)
		{
			variableStruct.name = varName;
			variableStruct.functionName = combinedFunction;
			variableStruct.type = "null";
			variableStruct.value = "0";

			m_varInfo.Add(variableStruct);

			m_varCount++;
		}
	}

	//Step 2: if GDB is running, send a "whatis" command to get the type.
	if( notFound &&
		(m_status == DEBUG_BREAK || m_status == DEBUG_WAIT))
	{
		sendWhat();

		m_classStatus = GET_WHAT;
	}
}

//////////////////////////////////////////////////////////////////////////////
///  public setVar
///  <-not implemented->
///
///  @param  varName   wxString  <>
///  @param  newValue  wxString  <>
///  @param  funcName  wxString  <>
///  @param  className wxString  <>
///
///  @return void
///
///  @author Ben Carhart @date 04-23-2004
//////////////////////////////////////////////////////////////////////////////
void Debugger::setVar(wxString varName, wxString newValue, wxString funcName, wxString className)
{
}

//////////////////////////////////////////////////////////////////////////////
///  private sendWhat
///  <part 1 of a 3 step internal process for watching variables.
///   -sends a [GDB SPECIFIC] command to get each stored variable's type>
///
///  @return void
///
///  @author Ben Carhart @date 04-23-2004
//////////////////////////////////////////////////////////////////////////////
void Debugger::sendWhat()
{
	if(m_varCount > 0)
	{
		wxString tmp;
		m_command.Clear();
		for(int i = 0; i < m_varCount; i++)
		{
			tmp.Printf("whatis %s%s", m_varInfo[i].name.c_str(), m_returnChar.c_str());
			m_command.Append(tmp);
		}

		sendCommand(m_command);

		m_classStatus = GET_WHAT;
	}
}

//////////////////////////////////////////////////////////////////////////////
///  private sendPrint
///  <Part 2 of a 3 step process.  Part 3 is in [onProcessOutput()].
///   -attempts to parse the [sendWhat()] commands and assigns them.  Then it
///    sends a "print varName" for each internal variable, paying attention
///    to special cases [GDB SPECIFIC]>
///
///  @param  fromGDB wxString  <full GDB output>
///
///  @return void
///
///  @author Ben Carhart @date 04-23-2004
//////////////////////////////////////////////////////////////////////////////
void Debugger::sendPrint(wxString fromGDB)
{
	bool watchStatus = ParseVariableTypes(fromGDB);


	if(!watchStatus)
	{
		wxString tmp;
		m_command.Clear();
		for(int i = 0; i < m_varCount; i++)
		{
			wxString name = m_varInfo[i].name;

			//special cases:
			// type==string: append "._M_dataplus._M_p" to format the string
			// type==[xxx]*: pre-pend "*" to de-reference the pointer type

			if(m_varInfo[i].type == "string")
			{
				name += "._M_dataplus._M_p";
			}

			if(m_varInfo[i].type.Find("*") != -1)
			{
				//name.Prepend("*");
				tmp.Printf("set print address on%sprint %s%sset print address off%s",
							m_returnChar, name, m_returnChar, m_returnChar);
			}
			else
			{
				tmp.Printf("print %s%s", name, m_returnChar);
			}

			m_command.Append(tmp);
		}

		sendCommand(m_command);

		m_classStatus = GET_PRINT;
	}
}

bool Debugger::ParseVariableTypes( wxString &fromGDB )
{
	wxString singleLine;
	wxArrayString fromWatch, ignoreVars;
	bool watchStatus = false;
	int lineBreak = 0, endQuote = 0, fromWatchIndex = 0, ampIdx = -1;
	int promptIndex = -1;
	bool singleLineItem = false;
	wxRegEx reTypeFinder(wxString("type = (\\(*([[:alnum:]]|_)+([[:blank:]]|\\*|&|\\[|]|[[:digit:]])*\\)*)"));

	wxArrayString outputLines = wxStringTokenize(fromGDB, "`");

	for(int i = 0; i < outputLines.GetCount(); i++)
	{
		wxString outputLine = outputLines[i];
		if(reTypeFinder.Matches(outputLine))
		{
			wxString match = reTypeFinder.GetMatch(outputLine, 1);
			fromWatch.Add(match);
		}
		else if(outputLine.Mid(0,9) == "No symbol")
		{
			lineBreak = outputLine.Find("\"");
			singleLine = outputLine.Mid(lineBreak + 1);

			endQuote = outputLine.Find("\"");
			singleLine = outputLine.Mid(0, endQuote);

			ignoreVars.Add(outputLine);
		}
		else if(outputLine.Mid(0,9) == "Attempt t")
		{
			fromWatch.Add("error");
		}
	}


	do
	{
		lineBreak = fromGDB.Find("\n");

		if(lineBreak == -1)
		{
			fromGDB = PROMPT_CHAR;
		}
		else
		{
			singleLine = fromGDB.Mid(0, lineBreak);
			singleLine = singleLine.BeforeFirst('\r');

			fromGDB = fromGDB.AfterFirst('\n');

			if(singleLine.Mid(0,4) == "type")
			{
				lineBreak = 2 + singleLine.Find("= ");
				singleLine = singleLine.Mid(lineBreak);


				ampIdx = singleLine.Find(" &");
				if(ampIdx != -1)
				{
					singleLine = singleLine.Remove(ampIdx);
					singleLine.Append("&");
				}

				ampIdx = singleLine.Find(" *");
				if(ampIdx != -1)
				{
					singleLine = singleLine.Remove(ampIdx);
					singleLine.Append("*");
				}

				//~~DEBUG~~//
				//wxLogDebug("\n--sendPrint: singleLine added: "+singleLine+"\n");

				fromWatch.Add(singleLine);
			}
			else if(singleLine.Mid(0,9) == "No symbol")
			{
				lineBreak = singleLine.Find("\"");
				singleLine = singleLine.Mid(lineBreak + 1);

				endQuote = singleLine.Find("\"");
				singleLine = singleLine.Mid(0, endQuote);

				ignoreVars.Add(singleLine);
			}
			else if(singleLine.Mid(0,9) == "Attempt t")
			{
				fromWatch.Add("error");
			}

		}
	}while(fromGDB != PROMPT_CHAR);


	if(!fromWatch.IsEmpty())
	{
		for(int i = 0; i < m_varCount; i++)
		{
			if(ignoreVars.Index(m_varInfo[i].name) == wxNOT_FOUND)
			{
				//current variable is not in the "ignoreVars" list...
				if(fromWatchIndex < (int)fromWatch.GetCount())
				{
					//~~DEBUG CODE~~//
					//wxLogDebug("\n--sendPrint: type set: "+fromWatch[fromWatchIndex]+"\n");

					m_varInfo[i].type = fromWatch[fromWatchIndex];

					fromWatchIndex++;

					//illegal variable name watch
					if(m_varInfo[i].type == "error")
					{
						m_varInfo.RemoveAt(i);
						m_varCount--;
					}
				}//testing "fromWatchIndex"
				else
				{
					//if we're down here, then there are more variables than there are
					//printed outputs.  So we by-pass the output in hopes that the
					//next few "whatis" statements come through
					watchStatus = true;
				}
			}//end variable ignore existance check
		}//end for
	}

	return watchStatus;
}

//////////////////////////////////////////////////////////////////////////////
///  public removeVar
///  <removes a variable being watched from my internal storage structure>
///
///  @param  varName   wxString  <the var name to remove>
///  @param  funcName  wxString  <-not used- specifics about the var>
///  @param  className wxString  <-not used- specifics about the var>
///
///  @return void
///
///  @author Ben Carhart @date 04-23-2004
//////////////////////////////////////////////////////////////////////////////
void Debugger::removeVar(wxString varName, wxString funcName, wxString className)
{
	//some added verbosity
	bool notFound = true,
		 found = false;

	if(m_varCount != 0)
	{
		int index = 0;
		for(index = 0; (index < m_varCount) && (notFound); index++)
		{
			if(m_varInfo[index].name == varName)
			{
				notFound = false;
				found = true;
			}
		}

		if(found)
		{
			m_varInfo.RemoveAt(index - 1);
			m_varCount--;
		}
	}//end varCount if
}

//////////////////////////////////////////////////////////////////////////////
///  private parsePrintOutput
///  <part of the part 3 of a 3 step process for watching variables
///   -attempts to parse full GDB output for the results of "sendPrint()"
///   author's note: it's a big friggin function with a 7 - 8 scope depth>
///
///  @param  fromGDB  wxString        <FULL GDB output>
///  @param  varValue wxArrayString & <parsed values to send back to the GUI>
///
///  @return bool     <returns wether a successful parse has occured>
///
///  @author Ben Carhart @date 04-23-2004
//////////////////////////////////////////////////////////////////////////////
bool Debugger::parsePrintOutput(wxString fromGDB, wxArrayString &variableValues)
{
	wxString singleLine, match;
	wxArrayString fromWatch, ignoreVars;
	int lineBreak = 0, endQuote = 0, fromWatchIndex = 0;
	bool singleLineItem = false;
	bool parseError = false, stayIn = true;

	wxArrayString outputLines = wxStringTokenize(fromGDB, "`");
	wxRegEx reVariableValue("\\$[[:digit:]]+ = (.+)");


	/* TODO:
	 * Tokenizing successfully splits things up into the relevant lines. But, now
	 * we need to deal with the various items.  It might be possible to ditch
	 * regex value parsing entirely, and just display everything after the "$N = ".
	 * Also, for classes, it would be useful to display any internal members
	 * hierarchically.  I _should_ be able to split things up based on the {}'s,
	 * store them using that STL tree class I found, and display them using
	 * wxTreeListCtrl as a replacement for the listview.
	 */

	int variableIndex = 0;
	for(int i = 0; i < outputLines.GetCount(); i++)
	{
		wxString outputLine = outputLines[i];

		if(outputLine.StartsWith("print"))
		{
			int dollarIndex = outputLine.Index(wxT("$"));

			if(dollarIndex != -1)
			{
				outputLine = outputLine.Mid(dollarIndex);
			}
		}


		if(outputLine.StartsWith("No symbol"))
		{
			variableValues.Add(outputLine);
			variableIndex++;
		}
		else if(reVariableValue.Matches(outputLine))
		{
			wxString variableValue = reVariableValue.GetMatch(outputLine, 1);
			variableValues.Add(variableValue);
			variableIndex++;
		}

		int q = i;
	}

	return !parseError;
}
//END VARIABLE FUNCTIONALITY

//MISC FUNCTIONS:
//---------------
//These funcions are random public thingies that don't qualify under any other
//heading.  "stop()" and "kill()" are listed as "misc" but i include those in
//step functionality since those affect the running of this class.

//////////////////////////////////////////////////////////////////////////////
///  public sendCustomCommand
///  <for the user to send a special custom command to GDB
///   -may not be hooked up->
///
///  @param  cust wxString  <the command to send>
///
///  @return void
///
///  @author Ben Carhart @date 04-23-2004
//////////////////////////////////////////////////////////////////////////////
void Debugger::sendCustomCommand(wxString cust)
{
	if(m_procLives == true)
	{
		cust.Append(m_returnChar);
		sendCommand(cust);
	}
}

//////////////////////////////////////////////////////////////////////////////
///  public getProcOutput
///  <gets X # of output entries from GDB.  -may not be hooked up->
///
///  @param  numEntries int  <return this many items from the output dump>
///
///  @return wxString   <all the items assembled in a SINGLE string>
///
///  @author Ben Carhart @date 04-23-2004
//////////////////////////////////////////////////////////////////////////////
wxString Debugger::getProcOutput(int numEntries = -1)
{
	wxString tmp = "";
	int outputCount = (int)m_fullOutput.GetCount();
	int loopStop = numEntries;
	if(outputCount > 0)
	{
		if(numEntries == -1)
		{loopStop = outputCount;}

		for(int i = 0; i < loopStop; i++)
		{tmp.Append(m_fullOutput[i]);}
	}

	return(tmp);
}


//PRIVATE FUNCTIONS:
//------------------

//The following 3 are from Dave Czechowski.  KUDOS!

//////////////////////////////////////////////////////////////////////////////
///  private onProcessOutputEvent
///  <the main "something happened in GDB... do something about it" function>
///
///  @param  e    ChameleonProcessEvent & <the process event>
///
///  @return void
///
///  @author Ben Carhart @date 04-23-2004
//////////////////////////////////////////////////////////////////////////////
void Debugger::onProcessOutputEvent(ChameleonProcessEvent &e)
{
	//variables to hold stuff for parsing...
	wxString tempHold, goBreakpoint;
	wxArrayString tmpArrayString;

	wxArrayString varNames, varValue, varType;

	wxString tempString;
	wxDebugEvent outputEvent;


	int classStatusBackup = 0,
		gdbNum = 0;
	long tmpLong;

	bool skipThrough = true,
		 keepParsing = false;

	//step parsing RegEx
	wxRegEx reCase1(" at (([[:alnum:]]|[[:blank:]]|\\.|/|_|:)+):([[:digit:]]+)");
	// needs the wxRE_ADVANCED in order to work properly, so can't just
	// assign this one as if it were a string
	wxRegEx reCase2("(\\n|^)(\\d+)[ \\t]+", wxRE_ADVANCED);

	//go parsing RegEx
	wxRegEx reStart("Initialization done");
	wxRegEx reGoCase("Breakpoint ([[:digit:]]+)");

	//breakpoint parsing RegEx
	wxRegEx reSetBreak("Breakpoint ([[:digit:]]+)");
	//!!END variables!!//

	///status = DEBUG_WAIT;

	//data now has the output...
	tempHold = e.GetString();
	m_data.Add(tempHold);

	//~~DEBUG CODE~~//
	wxLogDebug("DB output: %s", tempHold);

	//I don't think this code is effectual anymore... -B
	//REVISED: This code here is VITAL: DO NOT REMOVE unless you improve it -B
	//--//
	//1) See if the captured string has a [$] all by itself at the end
	//2) If so, begin parsing.
	//3) If not, wait for another output
	if(m_classStatus != START)
	{
		for(int i = 0; i < (int)m_data.GetCount() && skipThrough; i++)
		{
			tempHold = m_data[i];
			if(tempHold.Length() > 0 && tempHold.Last() == PROMPT_CHAR)
			{
				tempHold.Empty();

				for(int j = 0; j <= i; j++)
				{tempHold << m_data[j];}

				skipThrough = false;
			}
		}
	}
	else
	{
		skipThrough = false;
	}

	//check to see if something good was found.
	if(skipThrough)
	{
		classStatusBackup = m_classStatus;
		m_classStatus = WAITING;
	}
	//--//

	switch(m_classStatus)
	{
		case START:			//program start
			//this re-directs to where the output actually is that we want

			if(reStart.Matches(tempHold))
			{
				//the dope command "done" sent to GDB returns an error.
				//this means that all init. commands have gone through,
				//and we can start the program.
				m_status = DEBUG_STOPPED;
				go();
			}

			m_data.Empty();
			break;

		case E_BREAK:		//enable break
		case D_BREAK:		//disable break
		case K_BREAK:		//kill break
			//nothing to parse...
			m_data.Empty();
			break;

		case S_BREAK:		//set break
			if(reSetBreak.Matches(tempHold))
			{
				goBreakpoint = reSetBreak.GetMatch(tempHold, 1);

				goBreakpoint.ToLong(&tmpLong);
				gdbNum = (int)tmpLong;

				//~~DEBUG CODE~~//
				//wxLogDebug("--output_parse: setbreak: caught num=%d, gdb_num=%d--", gdbNum, (gdbBreakpointNum - 1));

				if(gdbNum != (m_gdbBreakpointNum - 1))
				{
					m_error.Printf("#s don't match: caught=%d internal=%d", gdbNum, (m_gdbBreakpointNum - 1));
					makeGenericError("Set_Break parse: ");


					outputEvent.SetId(ID_DEBUG_EXIT_ERROR);
					m_parentEventHandler->AddPendingEvent(outputEvent);
					stop(false);
				}
			}
			m_data.Empty();
			break;

		case RUN_TO_CURSOR:
		case CONTINUE:
		case GO:
		{
			keepParsing = checkOutputStream(tempHold);

			if(keepParsing)
			{
				if(m_classStatus == STOP)
				{
					outputEvent.SetId(ID_DEBUG_EXIT_NORMAL);
					m_parentEventHandler->AddPendingEvent(outputEvent);
					stop(false);
					break;
				}

				if(m_classStatus == RUN_TO_CURSOR || reGoCase.Matches(tempHold))
				{
					if(m_classStatus != RUN_TO_CURSOR)
					{
						goBreakpoint = reGoCase.GetMatch(tempHold, 1);
					}

					if(reCase1.Matches(tempHold))
					{
						m_Filename = reCase1.GetMatch(tempHold, 1);
						m_Linenumber = reCase1.GetMatch(tempHold, 3);

						m_status = DEBUG_BREAK;

						m_Linenumber.ToLong(&tmpLong);

						outputEvent.SetLineNumber((int)tmpLong);
						outputEvent.SetSourceFilename(m_Filename);
						outputEvent.SetId(ID_DEBUG_BREAKPOINT);
						m_parentEventHandler->AddPendingEvent(outputEvent);

						m_data.Empty();

						sendWhat();
					}
				}
				//given synchronous issues, an "else" here is invalid.  Why?
				//the fact that a command was sent can reach me before the
				//actual result of the command, which i need.
			}
			else
			{
				//something bad this way comes... aka Keeparsing = false
				outputEvent.SetId(ID_DEBUG_EXIT_ERROR);
				m_parentEventHandler->AddPendingEvent(outputEvent);
				stop(false);
			}
			m_data.Empty();
			break;
		}

		case JUMP:
		case SOFT_RESET:
		case HARD_RESET:
		case NEW_FILE:		//new file to debug
		case NEW_MODE:		//change in program mode
			flushBuffer();
			break;

		case NEW_PROC:		//change in process pointer
			flushBuffer();
			break;

		case VAR_ASSIGN:
			//the only time this status occurs is after a "print var = val"
			//First we check for text.  This means an invalid assignment
			//Next we check for the same things WATCH_VAR does...
			// so we can probably remove this "break" and let it flow into
			// WATCH_VAR...
			//or we could ignore what i wrote a coupl'a months ago ^^ and
			//let this flow into GET_PRINT.  :)  or an equiv.
			break;

		case GET_WHAT:
			sendPrint(tempHold);
			m_data.Empty();
			break;

		case GET_PRINT:
		{
			//varNames, varValue, varType
			bool dontskip = true;
			dontskip = parsePrintOutput(tempHold, varValue);

			if(dontskip)
			{
				for(int i = 0; i < m_varCount; i++)
				{
					varNames.Add(m_varInfo[i].name);
					varType.Add(m_varInfo[i].type);
				}

				m_classStatus = WAITING;

				if(m_varCount > 0)
				{
					outputEvent.SetVariableNames(varNames);
					outputEvent.SetVariableTypes(varType);
					outputEvent.SetVariableValues(varValue);
					outputEvent.SetId(ID_DEBUG_VARINFO);
					m_parentEventHandler->AddPendingEvent(outputEvent);
				}
			}

			m_data.Empty();
			break;
		}

		case WATCH_VAR:
			break;

 		case STEP:
		case STEP_OVER:
		case STEP_OUT:
		{

			//looking for this hierarchy:
			// 1) the word "Program" as this indicates a fatal error
			//  2) the pattern "f_name.ext:ln"
			//   3) a line number
			//case 1, reset statuses and send an error
			//case 2, update the filename and line number vars to send
			//case 3, update the line number and send
			//once one of these items is found, do a check on [topVarIndex > 0]
			//and update the array;

			//kudos to Mark for the RegEx!!
			bool noNewWhats = true;
			keepParsing = checkOutputStream(tempHold);

			//begin testing cases
			if(keepParsing)
			{
				if(m_classStatus == STOP)
				{
					outputEvent.SetId(ID_DEBUG_EXIT_NORMAL);
					m_parentEventHandler->AddPendingEvent(outputEvent);
					stop(false);
					break;
				}
				m_status = DEBUG_WAIT;
				if(reCase1.Matches(tempHold))
				{
					m_Filename = reCase1.GetMatch(tempHold, 1);
					m_Linenumber = reCase1.GetMatch(tempHold, 3);

					m_Linenumber.ToLong(&tmpLong);

					outputEvent.SetLineNumber((int)tmpLong);
					outputEvent.SetSourceFilename(m_Filename);
					outputEvent.SetId(ID_DEBUG_BREAKPOINT);
					m_parentEventHandler->AddPendingEvent(outputEvent);

					m_data.Empty();

					sendWhat();

					noNewWhats = false;
					m_status = DEBUG_BREAK;
				}
				else if(reCase2.Matches(tempHold))
				{
					m_Linenumber = reCase2.GetMatch(tempHold, 2);

					m_Linenumber.ToLong(&tmpLong);

					outputEvent.SetLineNumber((int)tmpLong);
					outputEvent.SetSourceFilename(m_Filename);
					outputEvent.SetId(ID_DEBUG_BREAKPOINT);
					m_parentEventHandler->AddPendingEvent(outputEvent);
				}
				//since we can pull right through, an "else" here is no good
			}
			else
			{
				outputEvent.SetId(ID_DEBUG_EXIT_ERROR);
				m_parentEventHandler->AddPendingEvent(outputEvent);
				stop(false);
			}

			m_data.Empty();

			if(noNewWhats)
			{
				sendPrint("");
			}
			break;
		}
		case WAITING:		//i'm waiting...
			m_classStatus = classStatusBackup;
			break;

		case STOP:
		{
			if(m_procLives)
			{
				m_status = DEBUG_ERROR;
				m_classStatus = STOP;
				m_error = "Extreme debug error OR quit-command sent before output finished: received output with ClassStatus=STOP.";
				makeGenericError("See previous error messages (if any)");

				outputEvent.SetId(ID_DEBUG_EXIT_ERROR);
				m_parentEventHandler->AddPendingEvent(outputEvent);
				stop(false);
			}
			m_data.Empty();
			break;
		}

		default:
			m_status = DEBUG_ERROR;
			m_classStatus = STOP;
			m_error = "[classStatus] value not recognized";
			makeGenericError("failure on output parse:");

			outputEvent.SetId(ID_DEBUG_EXIT_ERROR);
			m_parentEventHandler->AddPendingEvent(outputEvent);
			stop(false);

			m_data.Empty();
	}
}

//////////////////////////////////////////////////////////////////////////////
///  private checkOutputStream
///  <checks the process output for special tags that indicate a fatal / nice
///   program exit.  If neither, return's true with an UNEDITED [classStatus]>
///
///  @param  stream wxString  <the full process output>
///
///  @return bool   <[classStatus] is also modded if a successful quit has
///                  occurred.  Returns wether it's safe to continue parsing>
///
///  @author Ben Carhart @date 04-23-2004
//////////////////////////////////////////////////////////////////////////////
bool Debugger::checkOutputStream(wxString stream)
{
	wxRegEx reCase1("at (([[:alnum:]]|[[:blank:]]|\\.)+):([[:digit:]]+)");
	wxString thirdCase = stream;

	wxString prog = "Program ";
	int progIndex = thirdCase.Find(prog);

	if(progIndex != -1)
	{
		thirdCase.Remove(0,progIndex + prog.Len());
		if(thirdCase.Left(8) == "received")
		{
			int newlineIndex = thirdCase.find_first_of("\n");
			thirdCase.Remove(0, newlineIndex);

			//if we've gotten here, the program has received a message of some type.
			//either way, not good for further parsing.
			if(reCase1.Matches(thirdCase))
			{
				m_error = "Received bad program signal.  RegEx: " + reCase1.GetMatch(thirdCase, 1) + ", " + reCase1.GetMatch(thirdCase, 3);
				makeGenericError("checkOutputStream:  ");
				return(false);
			}
			else
			{
				m_error = "Received bad program signal.  No RegEx.";
				makeGenericError("checkOutputStream:  ");
				return(false);
			}
		}
		else if(thirdCase.Left(6) == "exited")
		{
			thirdCase.Remove(0,7);
			if(thirdCase.Matches("normal*"))
			{
				m_status = DEBUG_STOPPED;
				m_classStatus = STOP;
				return(true);
			}
			else
			{
				m_error.Printf("Program terminated abnormally.  ClassStatus %d", m_classStatus);
				makeGenericError("checkOutputStream: ");
				return(false);
			}
		}
		else
		{
			m_error.Printf("Unknown program signal. ClassStatus %d", m_classStatus);
			makeGenericError("checkOutputStream: ");
			return(false);
		}
	}

	return(true);
}

//////////////////////////////////////////////////////////////////////////////
///  private onProcessErrOutEvent
///  <something related to an error has occured with the process>
///
///  @param  e    ChameleonProcessEvent & <the process raising the error>
///
///  @return void
///
///  @author Ben Carhart @date 04-23-2004
//////////////////////////////////////////////////////////////////////////////
void Debugger::onProcessErrOutEvent(ChameleonProcessEvent &e)
{
	m_status = DEBUG_ERROR;

	//error now has the output
	m_error = e.GetString();
	addErrorHist("Event-Generated error");

	//DEBUG CODE//
	//wxLogDebug("DB error event: %s", error);
}

//////////////////////////////////////////////////////////////////////////////
///  private onProcessTermEvent
///  <process has terminated>
///
///  @param  e    ChameleonProcessEvent & <event from the process>
///
///  @return void
///
///  @author Ben Carhart @date 04-23-2004
//////////////////////////////////////////////////////////////////////////////
void Debugger::onProcessTermEvent(ChameleonProcessEvent &e)
{
	//the process has been killed (aka GDB quit for some reason)
	m_status = DEBUG_DEAD;
	m_classStatus = STOP;
	m_procLives = false;

	flushBuffer();
	flushPrivateVar();
}

//////////////////////////////////////////////////////////////////////////////
///  private flushBuffer
///  <a formal method of erasing the private but global array that holds proc
///   output>
///
///  @return void
///
///  @author Ben Carhart @date 04-23-2004
//////////////////////////////////////////////////////////////////////////////
void Debugger::flushBuffer()
{
	m_data.Clear();
}

//////////////////////////////////////////////////////////////////////////////
///  private flushPrivateVar
///  <erases all private data members (assuming i remembered them all!)>
///
///  @return void
///
///  @author Ben Carhart @date 04-23-2004
//////////////////////////////////////////////////////////////////////////////
void Debugger::flushPrivateVar()
{
	m_isRemote = false;
	m_procLives = false;
	m_fileIsSet = false;
	m_currFile = "";
	m_pid = 0;
	m_numBreakpoints = 0;
	m_gdbBreakpointNum = 1;
	m_lineToNum.empty();

	m_commandHistory.Empty();
	m_histCount = 0;
	m_error = "";
	errorHist.Empty();
	m_errorCount = 0;
	m_command = "";
	m_returnChar = "";
	m_data.Empty();
}

//////////////////////////////////////////////////////////////////////////////
///  private sendCommand
///  <sends a text block to the process (which essentially sends commands).
///   ALL COMMANDS ARE LOGGED>
///
///  @param  send wxString  <the text to send>
///
///  @return void
///
///  @author Ben Carhart @date 04-23-2004
//////////////////////////////////////////////////////////////////////////////
void Debugger::sendCommand(wxString send)
{
	if(m_procLives)
	{
		wxLogDebug("DB sent: %s", send);
		m_streamOut->WriteString(send);
		send = send + "<- sent";

		//DEBUG CODE//

	}
	else
	{
		send = send + "<- NOT SENT: PROC DEAD";
	}
	updateHistory(send);
}

//////////////////////////////////////////////////////////////////////////////
///  private makeGenericError
///  <a private method to set the class in an error state while adding an
///   error message from a given comment into the error history>
///
///  @param  comment wxString  <the comment to send to [addErrorHist()]>
///
///  @return void
///
///  @author Ben Carhart @date 04-23-2004
//////////////////////////////////////////////////////////////////////////////
void Debugger::makeGenericError(wxString comment)
{
	m_status = DEBUG_ERROR;	//if not already set...
	addErrorHist(comment);
}

//addErrorHist(): updates the error history
//////////////////////////////////////////////////////////////////////////////
///  private addErrorHist
///  <associated with [makeGenericError()] or can be called alone: adds a
///   given comment + error to the internal history dump.
///   NOTE: This does NOT set the error message!!>
///
///  @param  comment wxString  <the comment to append the error>
///
///  @return void
///
///  @author Ben Carhart @date 04-23-2004
//////////////////////////////////////////////////////////////////////////////
void Debugger::addErrorHist(wxString comment)
{
	m_errorCount++;
	errorHist.Add(comment + "-> " + m_error);
}

//updateHistory: updates the history storage and pointer
//////////////////////////////////////////////////////////////////////////////
///  private updateHistory
///  <called automatically after every [sendCommand()] call; appends the sent
///   command to the command dump>
///
///  @param  addMe wxString  <item to add>
///
///  @return void
///
///  @author Ben Carhart @date 04-23-2004
//////////////////////////////////////////////////////////////////////////////
void Debugger::updateHistory(wxString addMe)
{
	m_commandHistory.Add(addMe);
	m_histCount++;
}

//////////////////////////////////////////////////////////////////////////////
///  private getHistoryItem
///  <a private method for getting X number of history items>
///
///  @param  offset   int  <how many elements back in the history to grab>
///
///  @return wxString <returns the SINGLE element grabbed from the offset>
///
///  @author Ben Carhart @date 04-23-2004
//////////////////////////////////////////////////////////////////////////////
wxString Debugger::getHistoryItem(int offset = 0)
{
	if(offset > m_histCount)
	{
		offset = m_histCount;
	}
	return(m_commandHistory[offset]);
}
//eof
