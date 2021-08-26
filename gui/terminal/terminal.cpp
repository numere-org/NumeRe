/*****************************************************************************
    NumeRe: Framework fuer Numerische Rechnungen
    Copyright (C) 2018  Erik Haenel et al.

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


#ifdef __GNUG__
#pragma implementation "wxterm.h"
#endif

// For compilers that support precompilation, includes "wx/wx.h".
#include "wx/wxprec.h"

#ifdef __BORLANDC__
#pragma hdrstop
#endif

// for all others, include the necessary headers (this file is usually all you
// need because it includes almost all "standard" wxWindows headers
#ifndef WX_PRECOMP
#include "wx/wx.h"
#endif

#include <wx/timer.h>

#include <ctype.h>

#include "gterm.hpp"
#include "terminal.hpp"
#include <wx/clipbrd.h>
#include <wx/dataobj.h>
#include "../globals.hpp"


void WinMessageBeep();

#define CURSOR_BLINK_DEFAULT_TIMEOUT	500
#define CURSOR_BLINK_MAX_TIMEOUT	2000
#define KERNEL_THREAD_STACK_SIZE 4194304 // Bytes

BEGIN_EVENT_TABLE(NumeReTerminal, wxWindow)
	EVT_PAINT						(NumeReTerminal::OnPaint)
	EVT_CHAR						(NumeReTerminal::OnChar)
	EVT_LEFT_DOWN					(NumeReTerminal::OnLeftDown)
	EVT_MOUSE_CAPTURE_LOST          (NumeReTerminal::OnLoseMouseCapture)
	EVT_LEFT_UP					    (NumeReTerminal::OnLeftUp)
	EVT_MOTION					    (NumeReTerminal::OnMouseMove)
	EVT_ENTER_WINDOW                (NumeReTerminal::OnEnter)
	EVT_TIMER						(-1, NumeReTerminal::OnTimer)
	EVT_KEY_DOWN                    (NumeReTerminal::OnKeyDown)
	EVT_SIZE						(NumeReTerminal::OnSize)
	EVT_SET_FOCUS					(NumeReTerminal::OnGainFocus)
	EVT_KILL_FOCUS				    (NumeReTerminal::OnLoseFocus)
	EVT_CLOSE                       (NumeReTerminal::OnClose)
END_EVENT_TABLE()


/////////////////////////////////////////////////
/// \brief Terminal constructor.
///
/// \param parent wxWindow*
/// \param id wxWindowID
/// \param _option Options*
/// \param sPath const wxString&
/// \param pos const wxPoint&
/// \param width int
/// \param height int
/// \param name const wxString&
///
/////////////////////////////////////////////////
NumeReTerminal::NumeReTerminal(wxWindow* parent, wxWindowID id, Options* _option, const wxString& sPath, const wxPoint& pos, int width, int height, const wxString& name) : wxWindow(parent, id, pos, wxSize(-1, -1), wxWANTS_CHARS, name), GenericTerminal(width, height)
{
    // Bind the thread update event to the corresponding handler function
	Bind(wxEVT_THREAD, &NumeReTerminal::OnThreadUpdate, this);

	m_init = 1;

	// Initialize the terminal member variables
	m_inUpdateSize = false;
	m_isActive = false;
	m_isBusy = false;
	m_scrollBarWidth = wxSystemSettings::GetMetric(wxSYS_VSCROLL_ARROW_X);
	m_curDC = nullptr;
	m_charsInLine = width;
	m_linesDisplayed = height;
	m_selecting = false;
	m_selx1 = m_sely1 = m_selx2 = m_sely2 = 0;
	m_marking = false;
	m_curX = -1;
	m_curY = -1;
	m_curBlinkRate = CURSOR_BLINK_DEFAULT_TIMEOUT;
	m_timer.SetOwner(this);
	m_boldStyle = FONT;
	m_options = _option;
	m_width = width;
	m_height = height;
	m_bitmap = nullptr;
	m_wxParent = nullptr;

	/*
	**  Determine window size from current font
	*/
	wxClientDC
	dc(this);
    wxFont monospacedFont(8, wxMODERN, wxNORMAL, wxNORMAL, false, "Consolas");//10
	SetFont(monospacedFont);
	// Initialize the relevant fonts
	m_normalFont = GetFont();
	m_underlinedFont = GetFont();
	m_underlinedFont.SetUnderlined(true);
	m_boldFont = GetFont();
	m_boldFont.SetWeight(wxBOLD);
	m_boldUnderlinedFont = m_boldFont;
	m_boldUnderlinedFont.SetUnderlined(true);
	SetCursor(wxCursor(wxCURSOR_IBEAM));

    // Start the kernel
	_kernel.StartUp(this, sPath.ToStdString(), getSyntax()->getFunctions());
	m_options->copySettings(_kernel.getKernelSettings());

	// Update the terminal colors
	UpdateColors();

    // Calculate text extents
	int w, h;
	dc.SetFont(m_boldFont);
	dc.GetTextExtent("M", &m_charWidth, &h); // EKHL: Changed because Height made no sense
	dc.GetTextExtent("My", &w, &m_charHeight);

	SetClientSize(m_charsInLine * 8, m_linesDisplayed * 16);
	// 10pt Courier New is 8 pixels wide and 16 pixels high... set up
	// a default client size to match
	UpdateSize();

	// Start the cursor blink rate timer
	if (m_curBlinkRate)
		m_timer.Start(m_curBlinkRate);

#ifdef DO_LOG
    // Activate logging
	wxLog::SetActiveTarget(new wxLogWindow(this, "Logger"));
#endif

    // Start the kernel thread
	StartKernelTask();

	// Initialize the kernel-specific veriables
	m_KernelStatus = NumeReKernel::NUMERE_DONE;
	m_bCommandAvailable = false;
	m_bTableEditAvailable = false;
	m_bTableEditCanceled = false;
	m_nDebuggerCode = 0;
	m_sCommandLine = "";
	m_sAnswer = "";

	// Copy the plugin definitions to the syntax stylers
	{
		wxCriticalSectionLocker lock(m_kernelCS);
		getSyntax()->addPlugins(_kernel.getPluginCommands());
	}
	m_init = 0;
}


/////////////////////////////////////////////////
/// \brief Terminal destructor.
/////////////////////////////////////////////////
NumeReTerminal::~NumeReTerminal()
{
	if (m_bitmap)
	{
		m_memDC.SelectObject(wxNullBitmap);
		delete m_bitmap;
	}
}


/////////////////////////////////////////////////
/// \brief Returns the standard paths as a STL
/// vector.
///
/// \return std::vector<std::string>
///
/////////////////////////////////////////////////
std::vector<std::string> NumeReTerminal::getPathSettings()
{
	wxCriticalSectionLocker lock(m_kernelCS);
	vector<string> vPaths = _kernel.getPathSettings();
	return vPaths;
}


/////////////////////////////////////////////////
/// \brief Returns the installed plugins as a
/// STL vector.
///
/// \return std::vector<std::string>
///
/////////////////////////////////////////////////
std::vector<std::string> NumeReTerminal::getInstalledPackages()
{
    wxCriticalSectionLocker lock(m_kernelCS);
    return _kernel.getInstalledPackages();
}


/////////////////////////////////////////////////
/// \brief Returns the menu map of the installed
/// plugins as a STL map.
///
/// \return std::map<std::string, std::string>
///
/////////////////////////////////////////////////
std::map<std::string, std::string> NumeReTerminal::getMenuMap()
{
    wxCriticalSectionLocker lock(m_kernelCS);
    return _kernel.getMenuMap();
}


/////////////////////////////////////////////////
/// \brief Passes a table (as a container) to the
/// kernel.
///
/// \param _table NumeRe::Table
/// \return void
///
/////////////////////////////////////////////////
void NumeReTerminal::passEditedTable(NumeRe::Table _table)
{
	wxCriticalSectionLocker lock(m_kernelCS);
	_kernel.table = _table;
	m_bTableEditAvailable = true;
}


/////////////////////////////////////////////////
/// \brief This member function adds a breakpoint
/// to the passed file at the indicated line
/// number.
///
/// \param _sFilename const string&
/// \param nLine size_t
/// \return void
///
/////////////////////////////////////////////////
void NumeReTerminal::addBreakpoint(const string& _sFilename, size_t nLine)
{
    wxCriticalSectionLocker lock(m_kernelCS);
    _kernel.getDebugger().getBreakpointManager().addBreakpoint(_sFilename, nLine);
}


/////////////////////////////////////////////////
/// \brief This member function removes a
/// breakpoint from the passed file at the
/// indicated line number.
///
/// \param _sFilename const string&
/// \param nLine size_t
/// \return void
///
/////////////////////////////////////////////////
void NumeReTerminal::removeBreakpoint(const string& _sFilename, size_t nLine)
{
    wxCriticalSectionLocker lock(m_kernelCS);
    _kernel.getDebugger().getBreakpointManager().removeBreakpoint(_sFilename, nLine);
}


/////////////////////////////////////////////////
/// \brief This member function removes all
/// breakpoints from the passed file.
///
/// \param _sFilename const string&
/// \return void
///
/////////////////////////////////////////////////
void NumeReTerminal::clearBreakpoints(const string& _sFilename)
{
    wxCriticalSectionLocker lock(m_kernelCS);
    _kernel.getDebugger().getBreakpointManager().clearBreakpoints(_sFilename);
}


/////////////////////////////////////////////////
/// \brief Gets the desired documentation article
/// as a HTML string.
///
/// \param sCommand const string&
/// \return string
///
/////////////////////////////////////////////////
string NumeReTerminal::getDocumentation(const string& sCommand)
{
	wxCriticalSectionLocker lock(m_kernelCS);
	return _kernel.getDocumentation(sCommand);
}


/////////////////////////////////////////////////
/// \brief Gets the contents of the documentation
/// index as a vector.
///
/// \return vector<string>
///
/////////////////////////////////////////////////
vector<string> NumeReTerminal::getDocIndex()
{
    wxCriticalSectionLocker lock(m_kernelCS);
    return _kernel.getDocIndex();
}


/////////////////////////////////////////////////
/// \brief This will return the language strings
/// for the plugins used by the language class
/// for filling the symbols tree.
///
/// \return map<string, string>
///
/////////////////////////////////////////////////
map<string, string> NumeReTerminal::getPluginLanguageStrings()
{
	wxCriticalSectionLocker lock(m_kernelCS);
	return _kernel.getPluginLanguageStrings();
}


/////////////////////////////////////////////////
/// \brief This will return the language strings
/// for the custom defined functions used by the
/// language class for filling the symbols tree.
///
/// \return map<string, string>
///
/////////////////////////////////////////////////
map<string, string> NumeReTerminal::getFunctionLanguageStrings()
{
	wxCriticalSectionLocker lock(m_kernelCS);
	return _kernel.getFunctionLanguageStrings();
}


/////////////////////////////////////////////////
/// \brief This will return the variable list
/// from the kernel to be shown in the variable
/// viewer.
///
/// \return NumeReVariables
///
/////////////////////////////////////////////////
NumeReVariables NumeReTerminal::getVariableList()
{
    wxCriticalSectionLocker lock(m_kernelCS);
    return _kernel.getVariableList();
}


/////////////////////////////////////////////////
/// \brief This will return a copy of the
/// internal settings object of the kernel.
///
/// \return Settings
///
/////////////////////////////////////////////////
Settings NumeReTerminal::getKernelSettings()
{
	wxCriticalSectionLocker lock(m_kernelCS);
	Settings _option(_kernel.getKernelSettings());
	return _option;
}


/////////////////////////////////////////////////
/// \brief This will pass the new kernel settings
/// to the kernel.
///
/// \param _settings const Settings&
/// \return void
///
/////////////////////////////////////////////////
void NumeReTerminal::setKernelSettings(const Settings& _settings)
{
	wxCriticalSectionLocker lock(m_kernelCS);
	_kernel.setKernelSettings(_settings);
}


/////////////////////////////////////////////////
/// \brief This member function will start the
/// managed, second thread, in which the kernel
/// will operate.
///
/// \return void
///
/////////////////////////////////////////////////
void NumeReTerminal::StartKernelTask()
{
	if (CreateThread(wxTHREAD_JOINABLE, KERNEL_THREAD_STACK_SIZE) != wxTHREAD_NO_ERROR)
	{
		wxLogError("Could not create kernel thread!");
		return;
	}
	if (GetThread()->Run() != wxTHREAD_NO_ERROR)
	{
		wxLogError("Could not run the kernel thread!");
		return;
	}
}


/////////////////////////////////////////////////
/// \brief This is the main thread function and
/// will be called repeatedly from the wxWidgets
/// library.
///
/// \return wxThread::ExitCode
///
/////////////////////////////////////////////////
wxThread::ExitCode NumeReTerminal::Entry()
{
	string sCommand;
	bool bCommandAvailable;
	bool updateLibrary;
	_kernel.printVersionInfo();

	// Test repeatedly, whether the thread shall terminate
	while (!GetThread()->TestDestroy())
	{
	    // Perform this loop only 10 times a second
		Sleep(100);

		// start critical section for reading the command and the boolean
		{
			wxCriticalSectionLocker lock(m_kernelCS);

			bCommandAvailable = m_bCommandAvailable;
			sCommand = m_sCommandLine;
			m_bCommandAvailable = false;

			updateLibrary = m_updateProcedureLibrary;
			m_updateProcedureLibrary = false;
			m_sCommandLine.clear();

			if (!sCommand.length() && bCommandAvailable)
			{
				m_KernelStatus = NumeReKernel::NUMERE_PENDING;
				wxQueueEvent(GetEventHandler(), new wxThreadEvent());
				continue;
			}
		}

		// A command is available
		if (bCommandAvailable)
		{
			// This is the actual evaluating function. It is called from this second thread regularly (every 100ms) and
			// enters the function, if a command was passed to the terminal.
			m_KernelStatus = _kernel.MainLoop(sCommand);

			if (m_KernelStatus > 0) // these are valid status values (0 = error, -1 = quit)
			{
				wxCriticalSectionLocker lock(m_kernelCS);

				switch (m_KernelStatus)
				{
					case NumeReKernel::NUMERE_DONE:
						m_sAnswer = _kernel.ReadAnswer();
						break;
					default:
						break;
						//All others
				}
			}
			else if (m_KernelStatus == NumeReKernel::NUMERE_QUIT) //quit
			{
				break;
			}

			// Notify the event handler that there's an update
			wxQueueEvent(GetEventHandler(), new wxThreadEvent());
		}

		// During idle times so that these tasks don't interfere with the main evaluation routine
		// do the following:
		if (time(0) - _kernel.getLastSavedTime() >= _kernel.getAutosaveInterval())
			_kernel.Autosave(); // save the cache

		if (updateLibrary)
		{
		    // update the internal procedure library if needed
			NumeReKernel::ProcLibrary.updateLibrary();
		}
	}

	// The thread will terminate
	// Close the session and inform the thread handler
	_kernel.CloseSession();
	m_KernelStatus = NumeReKernel::NUMERE_QUIT;
	wxQueueEvent(GetEventHandler(), new wxThreadEvent());
	return (wxThread::ExitCode)0;
}


/////////////////////////////////////////////////
/// \brief This function forces the thread to
/// terminate so that the application may be shut
/// down.
///
/// \return void
///
/////////////////////////////////////////////////
void NumeReTerminal::EndKernelTask()
{
	if (GetThread() && GetThread()->IsRunning())
	{
		erase_line();
		GetThread()->Delete();
	}
}


/////////////////////////////////////////////////
/// \brief This function forces the thread to
/// terminate immediately. Should only occur in
/// situations, where we face a crash.
///
/// \param event wxCloseEvent&
/// \return void
///
/////////////////////////////////////////////////
void NumeReTerminal::OnClose(wxCloseEvent& event)
{
	if (GetThread() && GetThread()->IsRunning())
		GetThread()->Wait();
	wxMilliSleep(200);
	Destroy();
}


/////////////////////////////////////////////////
/// \brief This function is the thread update
/// event handler member function. Here are all
/// returned messages from the kernel and its
/// notifications processed.
///
/// \param event wxThreadEvent&
/// \return void
///
/////////////////////////////////////////////////
void NumeReTerminal::OnThreadUpdate(wxThreadEvent& event)
{
	bool Closing = false;
	bool changedSettings = false;
	bool done = false;
	bool refreshFunctionTree = false;
	queue<NumeReTask> taskQueue;
	string sAnswer = "";

	// Get the kernel status and read the variables
	// correspondingly
	{
		wxCriticalSectionLocker lock(m_kernelCS);
		// Always read the complete task queue
		if (_kernel.taskQueue.size())
            taskQueue.swap(_kernel.taskQueue);

		switch (m_KernelStatus)
		{
			case NumeReKernel::NUMERE_ANSWER_READ:
			case NumeReKernel::NUMERE_PENDING_SPECIAL:
				return;
			// fallthrough is intended
			case NumeReKernel::NUMERE_DONE:
				sAnswer = "|-> " + m_sAnswer + "\n|\n|<- ";
				done = true;
				break;
            // fallthrough is intended
			case NumeReKernel::NUMERE_ERROR:
			case NumeReKernel::NUMERE_DONE_KEYWORD:
				sAnswer = m_sAnswer + "|\n|<- ";
				done = true;
				break;
			case NumeReKernel::NUMERE_PENDING:
			    done = true;
				sAnswer = "|<- ";
				break;
			case NumeReKernel::NUMERE_QUIT:
				Closing = true;
				break;
			default:
				//All other cases
				sAnswer = m_sAnswer;
		}

		// clear the communication variables
		m_sAnswer.clear();
		changedSettings = _kernel.SettingsModified();
		m_KernelStatus = NumeReKernel::NUMERE_ANSWER_READ;
	}

	// Toggle the toolbar tools used for stopping and starting tasks
	if (done)
	{
		Ready();
	}

	// If the kernel asks the application to terminate
	// do this after 200msec
	if (Closing)
	{
		wxMilliSleep(300);
		m_wxParent->Close();
		return;
	}

    // Evaluate the task queue. It will contain also the transmitted
    // variables needed for the current task
    while (taskQueue.size())
    {
        // Get the first task and remove it from
        // the queue
        NumeReTask task = taskQueue.front();
        taskQueue.pop();

        // Evaluate the current task depending on its type
        switch (task.taskType)
        {
            case NumeReKernel::NUMERE_EDIT_FILE:
            {
                if (task.sString.find(".png") != string::npos
                        || task.sString.find(".jpg") != string::npos
                        || task.sString.find(".jpeg") != string::npos
                        || task.sString.find(".gif") != string::npos
                        || task.sString.find(".bmp") != string::npos)
                {
                    m_wxParent->openImage(wxFileName(task.sString));
                }
                else
                {
                    m_wxParent->OpenSourceFile(wxArrayString(1, task.sString), task.nLine, _kernel.ReadOpenFileFlag());
                }
                break;
            }
            case NumeReKernel::NUMERE_SHOW_WINDOW:
                Refresh();
                m_wxParent->showWindow(task.window);
                break;
            case NumeReKernel::NUMERE_OPEN_DOC:
                m_wxParent->ShowHelp(task.sString);
                break;
            case NumeReKernel::NUMERE_SHOW_TABLE:
                m_wxParent->openTable(task.table, task.sString);
                break;
            case NumeReKernel::NUMERE_SHOW_STRING_TABLE:
                m_wxParent->openTable(task.stringTable, task.sString);
                break;
            case NumeReKernel::NUMERE_EDIT_TABLE:
                m_wxParent->editTable(task.table, task.sString);
                break;
            case NumeReKernel::NUMERE_DEBUG_EVENT:
                m_wxParent->evaluateDebugInfo(task.vDebugEvent);
                break;
            case NumeReKernel::NUMERE_REFRESH_FUNCTIONTREE:
                refreshFunctionTree = true;
                break;
            case NumeReKernel::NUMERE_CLOSE_WINDOWS:
                m_wxParent->closeWindows((WindowType)task.nLine);
                break;
            case NumeReKernel::NUMERE_INSTALLATION_DONE:
                m_wxParent->notifyInstallationDone();
                break;
        }
    }

	// This boolean is true, if the user switched a setting directly in the kernel
	if (changedSettings)
	{
	    m_options->copySettings(getKernelSettings());
		m_wxParent->EvaluateOptions();
	}

	// Refresh now the function tree (will avoid multiple
    // refreshes in a single event loop).
	if (refreshFunctionTree)
    {
        getSyntax()->addPlugins(_kernel.getPluginCommands());
        m_wxParent->refreshFunctionTree();
    }

	// To ensure that the user is able to read the answer
	// scroll to the input location and process the kernel
	// message
	scrollToInput();

	if (sAnswer == "|\n|<- " && GetTM()->getPreviousLine() == "|")
        sAnswer = "|<- ";

	ProcessOutput(sAnswer.length(), sAnswer);
	Refresh();
}


//////////////////////////////////////////////////////////////////////////////
///  public SetFont
///  Sets the font for the terminal
///
///  @param  font const wxFont & The font to be used
///
///  @return bool Unused (returns true)
///
///  @author Derry Bryson @date 04-22-2004
//////////////////////////////////////////////////////////////////////////////
bool
NumeReTerminal::SetFont(const wxFont& font)
{
	m_init = 1;

    // Set the passed font to all internal member variables
	wxWindow::SetFont(font);
	m_normalFont = font;
	m_underlinedFont = font;
	m_underlinedFont.SetUnderlined(true);
	m_boldFont = GetFont();
	m_boldFont.SetWeight(wxBOLD);
	m_boldUnderlinedFont = m_boldFont;
	m_boldUnderlinedFont.SetUnderlined(true);

	m_init = 0;

	// Resize the terminal, because the new
	// font might have a new text extent
	ResizeTerminal(m_width, m_height);

	// Refresh the GUI element
	Refresh();

	return true;
}


//////////////////////////////////////////////////////////////////////////////
///  public GetDefColors
///  Gets the colors for the terminal from the internal Options object
///
///  @param  colors wxColour [] The colors that need to be assigned to
///  @param  boldStyle wxTerm::BOLDSTYLE The bold style used in the terminal
///
///  @return void
///
///  @author Derry Bryson @date 04-22-2004
//////////////////////////////////////////////////////////////////////////////
void
NumeReTerminal::GetDefColors(wxColour colors[16], NumeReTerminal::BOLDSTYLE boldStyle)
{
    // Set the correct bold style
	if (boldStyle == DEFAULT)
		boldStyle = m_boldStyle;

    // Beause the enumeration of the internal syntax and the syntax of the
    // terminal is not identical, we need to use this lookup function
    //
    // We could also use a simple array to get the correct index, however
    // using this switch with the correct enumerators is probably more safe.
	for (size_t i = 0; i <= NumeReSyntax::SYNTAX_METHODS; i++)
	{
		switch (i)
		{
			case 0:
				colors[0] = m_options->GetSyntaxStyle(Options::CONSOLE_STD).background;
				break;
			case NumeReSyntax::SYNTAX_COMMAND:
				colors[NumeReSyntax::SYNTAX_COMMAND] = m_options->GetSyntaxStyle(Options::COMMAND).foreground;
				break;
			case NumeReSyntax::SYNTAX_OPTION:
				colors[NumeReSyntax::SYNTAX_OPTION] = m_options->GetSyntaxStyle(Options::OPTION).foreground;
				break;
			case NumeReSyntax::SYNTAX_FUNCTION:
				colors[NumeReSyntax::SYNTAX_FUNCTION] = m_options->GetSyntaxStyle(Options::FUNCTION).foreground;
				break;
			case NumeReSyntax::SYNTAX_CONSTANT:
				colors[NumeReSyntax::SYNTAX_CONSTANT] = m_options->GetSyntaxStyle(Options::CONSTANT).foreground;
				break;
			case NumeReSyntax::SYNTAX_SPECIALVAL:
				colors[NumeReSyntax::SYNTAX_SPECIALVAL] = m_options->GetSyntaxStyle(Options::SPECIALVAL).foreground;
				break;
			case NumeReSyntax::SYNTAX_STRING:
				colors[NumeReSyntax::SYNTAX_STRING] = m_options->GetSyntaxStyle(Options::STRING).foreground;
				break;
			case NumeReSyntax::SYNTAX_STD:
				colors[NumeReSyntax::SYNTAX_STD] = m_options->GetSyntaxStyle(Options::CONSOLE_STD).foreground;
				break;
			case NumeReSyntax::SYNTAX_OPERATOR:
				colors[NumeReSyntax::SYNTAX_OPERATOR] = m_options->GetSyntaxStyle(Options::OPERATOR).foreground;
				break;
			case NumeReSyntax::SYNTAX_PROCEDURE:
				colors[NumeReSyntax::SYNTAX_PROCEDURE] = m_options->GetSyntaxStyle(Options::PROCEDURE).foreground;
				break;
			case NumeReSyntax::SYNTAX_NUMBER:
				colors[NumeReSyntax::SYNTAX_NUMBER] = m_options->GetSyntaxStyle(Options::NUMBER).foreground;
				break;
			case NumeReSyntax::SYNTAX_NPRC_COMMAND:
				colors[NumeReSyntax::SYNTAX_NPRC_COMMAND] = m_options->GetSyntaxStyle(Options::PROCEDURE_COMMAND).foreground;
				break;
			case NumeReSyntax::SYNTAX_METHODS:
				colors[NumeReSyntax::SYNTAX_METHODS] = m_options->GetSyntaxStyle(Options::METHODS).foreground;
				break;
		}
	}

	// remaining, free colours
	colors[13] = wxColour(255, 255, 0);
	colors[14] = wxColour(0, 255, 255);
	colors[15] = wxColour(0, 0, 0);
}


//////////////////////////////////////////////////////////////////////////////
///  public SetCursorBlinkRate
///  Sets how often the cursor blinks
///
///  @param  rate int  How many milliseconds between blinks
///
///  @return void
///
///  @author Derry Bryson @date 04-22-2004
//////////////////////////////////////////////////////////////////////////////
void
NumeReTerminal::SetCursorBlinkRate(int rate)
{
    // Ensure that the new rate is reasonable
	if (rate < 0 || rate > CURSOR_BLINK_MAX_TIMEOUT)
		return;

	m_init = 1;

	// If the new rate is different from the old rate
	// set it and restart the blinking timer
	if (rate != m_curBlinkRate)
	{
		m_curBlinkRate = rate;
		if (!m_curBlinkRate)
			m_timer.Stop();
		else
			m_timer.Start(m_curBlinkRate);
	}

	m_init = 0;
}


/////////////////////////////////////////////////
/// \brief Pass the entered command line to the
/// kernel.
///
/// This is called when the user hits "Enter". It
/// will get the current input line from the
/// internal buffer and sent it to the kernel.
///
/// \param sCommand const string&
/// \return void
///
/////////////////////////////////////////////////
void NumeReTerminal::pipe_command(const string& sCommand)
{
	wxCriticalSectionLocker lock(m_kernelCS);

    // the commandline from tm...
	m_sCommandLine = sCommand;

	// only add the line to the history, if the kernel isn't currently fetching a line with NumeReKernel::getline()
	if (!NumeReKernel::bGettingLine)
		m_wxParent->AddToHistory(sCommand);

	m_bCommandAvailable = true;
}


/////////////////////////////////////////////////
/// \brief  Pass the external command to the
/// kernel without printing it to the console.
///
/// \param command const string&
/// \param isEvent bool Do not add to the history
/// \return void
///
/////////////////////////////////////////////////
void NumeReTerminal::pass_command(const string& command, bool isEvent)
{
    // Don't do anything if the command is emoty
	if (!command.length())
		return;

#ifdef DO_LOG
    if (isEvent)
        wxLogDebug("Event: '%s'", command);
#endif

    // scroll to the input location
	scrollToInput();

	// erase the current line
	erase_line();

	Busy();

	// Set the new command to the kernel and log it
	wxCriticalSectionLocker lock(m_kernelCS);

	if (!isEvent)
        m_wxParent->AddToHistory(command);

    m_sCommandLine = command;
	m_bCommandAvailable = true;
}


/////////////////////////////////////////////////
/// \brief This function will return the named
/// table from the kernel to be shown in a GUI
/// window.
///
/// \param sTableName const string&
/// \return NumeRe::Table
///
/////////////////////////////////////////////////
NumeRe::Table NumeReTerminal::getTable(const string& sTableName)
{
    wxCriticalSectionLocker lock(m_kernelCS);
    return _kernel.getTable(sTableName);
}


/////////////////////////////////////////////////
/// \brief This member function will return the
/// named table containing strings.
///
/// \param sStringTableName const string&
/// \return NumeRe::Container<string>
///
/////////////////////////////////////////////////
NumeRe::Container<string> NumeReTerminal::getStringTable(const string& sStringTableName)
{
    wxCriticalSectionLocker lock(m_kernelCS);
    return _kernel.getStringTable(sStringTableName);
}


/////////////////////////////////////////////////
/// \brief Inform the kernel to stop the current
/// calculation. Used to handle the ESC key
/// press.
///
/// \return void
///
/////////////////////////////////////////////////
void NumeReTerminal::CancelCalculation()
{
	wxCriticalSectionLocker lock(m_kernelCS);
	_kernel.CancelCalculation();
}


//////////////////////////////////////////////////////////////////////////////
///  private OnChar
///  Handles user keyboard input and begins processing the server's response
///
///  @param  event wxKeyEvent & The generated key event
///
///  @return void
///
///  @author Derry Bryson @date 04-22-2004
//////////////////////////////////////////////////////////////////////////////
void
NumeReTerminal::OnChar(wxKeyEvent& event)
{
	if (!(GetMode() & PC) && event.AltDown())
		event.Skip();
	else
	{
		// if the user is scrolled up and they typed something, scroll
		// all the way to the bottom
		scrollToInput();

		int keyCode = (int)event.GetKeyCode();
		int len = 1;

		// Clear selection mode
		// can probably be modified to clear the selected input line
		if (HasSelection())
        {
            if (keyCode == WXK_DELETE || keyCode == WXK_BACK)
            {
                delSelected();
                ClearSelection();
                return;
            }
            else if (keyCode < WXK_START)
                delSelected();

			ClearSelection();
        }

        // Filter special keycodes
        if (filterKeyCodes(keyCode, event.ControlDown()))
            return;

		std::string buf = " ";
		buf[0] = (char)keyCode;

		GenericTerminal::resetAutoComp(RESETCURSOR | RESETTAB);

        wxClientDC dc(this);

        m_curDC = &dc;

        // Process the input line
        GenericTerminal::ProcessInput(len, buf);

        m_curDC = nullptr;
	}
}


/////////////////////////////////////////////////
/// \brief This private member function filters
/// special key codes and handles them.
///
/// \param keyCode int
/// \param ctrlDown bool
/// \return bool
///
/////////////////////////////////////////////////
bool NumeReTerminal::filterKeyCodes(int keyCode, bool ctrlDown)
{
    // Filter special keycodes
    switch (keyCode)
    {
        case WXK_RETURN:
            {
                GenericTerminal::resetAutoComp(RESETCURSOR | RESETTAB);
                string sCommand = GetTM()->getCurrentInputLine();
                GenericTerminal::cr();
                GenericTerminal::lf();
                GetTM()->ChangeEditableState();
                Refresh();
                Busy();
                pipe_command(sCommand);
                return true;
            }
        case WXK_BACK:
            GenericTerminal::resetAutoComp(RESETCURSOR | RESETTAB);
            if (GenericTerminal::bs())
                Refresh();

            return true;
        case WXK_TAB:
            GenericTerminal::resetAutoComp(RESETCURSOR);
            GenericTerminal::tab();
            Refresh();
            return true;
        case WXK_LEFT:
            GenericTerminal::resetAutoComp(RESETCURSOR | RESETTAB);
            if ((ctrlDown && GenericTerminal::ctrl_left()) || GenericTerminal::cursor_left())
            {
                Refresh();
            }
            return true;
        case WXK_RIGHT:
            GenericTerminal::resetAutoComp(RESETCURSOR | RESETTAB);
            if ((ctrlDown && GenericTerminal::ctrl_right()) || GenericTerminal::cursor_right())
            {
                Refresh();
            }
            return true;
        case WXK_UP:
            GenericTerminal::resetAutoComp(RESETTAB);
            if (GenericTerminal::cursor_up())
            {
                Refresh();
            }
            return true;
        case WXK_DOWN:
            GenericTerminal::resetAutoComp(RESETTAB);
            if (GenericTerminal::cursor_down())
            {
                Refresh();
            }
            return true;
        case WXK_HOME:
            GenericTerminal::resetAutoComp(RESETCURSOR | RESETTAB);
            if (GenericTerminal::home())
            {
                Refresh();
            }
            return true;
        case WXK_END:
            GenericTerminal::resetAutoComp(RESETCURSOR | RESETTAB);
            if (GenericTerminal::end())
            {
                Refresh();
            }
            return true;
        case WXK_DELETE:
            GenericTerminal::resetAutoComp(RESETCURSOR | RESETTAB);
            if (GenericTerminal::del())
                Refresh();

            return true;
    }

    // Filter out any other non-ASCII characters
    if (keyCode >= WXK_START || keyCode < WXK_SPACE)
        return true;

    // No special key code
    return false;
}


/////////////////////////////////////////////////
/// \brief This private member function scrolls
/// the terminal all the way down.
///
/// \return void
///
/////////////////////////////////////////////////
void NumeReTerminal::scrollToInput()
{
	if (GenericTerminal::IsScrolledUp())
	{
	    // Get the number of scrolled lines and scroll them down
		GenericTerminal::Scroll(GetTM()->GetNumLinesScrolled(), false);
		GenericTerminal::Update();
		Refresh();
	}
}


//////////////////////////////////////////////////////////////////////////////
///  private OnKeyDown
///  Used to handle copy-paste control sequences
///
///  @param  event wxKeyEvent & The generated key event
///
///  @return void
///
///  @author Derry Bryson @date 04-22-2004
//////////////////////////////////////////////////////////////////////////////
void
NumeReTerminal::OnKeyDown(wxKeyEvent& event)
{
	if (!(GetMode() & PC) && event.AltDown())
		event.Skip();
	else if (event.ControlDown() && event.ShiftDown())
	{
	    // The keycode will be in uppercase letters,
	    // if we require to push shift
		if (event.GetKeyCode() == 'C' || event.GetKeyCode() == 'V')
		{
		    // Copy or paste
			if (event.GetKeyCode() == 'C' && HasSelection())
			{
			    // Get the selection and store it in the clipboard
				wxString sSelection = GetSelection();

				if (!sSelection.length())
					return;

				if (wxTheClipboard->Open())
				{
					wxTheClipboard->SetData(new wxTextDataObject(sSelection));
					wxTheClipboard->Close();
				}
			}
			else if (event.GetKeyCode() == 'V')
			{
			    // Get the text from the clipboard and process it as new input
				if (wxTheClipboard->Open())
				{
					if (wxTheClipboard->IsSupported(wxDF_TEXT))
					{
						wxTextDataObject data;
						wxTheClipboard->GetData(data);
						NumeReTerminal::ProcessInput(data.GetTextLength(), data.GetText().ToStdString());
						Refresh();
					}

					wxTheClipboard->Close();
				}
			}

			return;
		}
	}
	else if (event.GetKeyCode() == WXK_ESCAPE)
    {
        if (m_isBusy)
            CancelCalculation();
        else
        {
            GenericTerminal::erase_usercontent_line();
            Refresh();
        }
    }
	else
		event.Skip();
}


//////////////////////////////////////////////////////////////////////////////
///  private OnPaint
///  Redraws the terminal widget
///
///  @param  event wxPaintEvent & The generated paint event
///
///  @return void
///
///  @author Derry Bryson @date 04-22-2004
//////////////////////////////////////////////////////////////////////////////
void
NumeReTerminal::OnPaint(wxPaintEvent& WXUNUSED(event))
{
	wxPaintDC dc(this);

#ifdef DO_LOG
	wxLogDebug("Painting");
#endif

    wxDC* backup = m_curDC;

	m_curDC = &dc;
	update_changes();
	m_curDC = backup;
}


//////////////////////////////////////////////////////////////////////////////
///  private OnLeftDown
///  Begins selection of terminal text
///
///  @param  event wxMouseEvent & The generated mouse event
///
///  @return void
///
///  @author Derry Bryson @date 04-22-2004
//////////////////////////////////////////////////////////////////////////////
void
NumeReTerminal::OnLeftDown(wxMouseEvent& event)
{
	SetFocus();
	ClearSelection();
	m_selx1 = m_selx2 = event.GetX() / m_charWidth;
	m_sely1 = m_sely2 = event.GetY() / m_charHeight;
	m_selecting = true;
	this->CaptureMouse();
}


/////////////////////////////////////////////////
/// \brief This member function handles the
/// "MouseCaptureLostEvent" and releases the
/// mouse.
///
/// \param event wxMouseCaptureLostEvent&
/// \return void
///
/////////////////////////////////////////////////
void NumeReTerminal::OnLoseMouseCapture(wxMouseCaptureLostEvent& event)
{
	if (this->GetCapture() == this)
	{
		m_selecting = false;
		this->ReleaseMouse();
		Refresh();
	}
}


//////////////////////////////////////////////////////////////////////////////
///  private OnLeftUp
///  Ends text selection
///
///  @param  event wxMouseEvent & The generated mouse event
///
///  @return void
///
///  @author Derry Bryson @date 04-22-2004
//////////////////////////////////////////////////////////////////////////////
void
NumeReTerminal::OnLeftUp(wxMouseEvent& event)
{
	m_selecting = false;
	if (this->GetCapture() == this)
	{
		this->ReleaseMouse();
		Refresh();
	}
	move_cursor_editable_area(m_selx2, m_sely2);
}


//////////////////////////////////////////////////////////////////////////////
///  private OnMouseMove
///  Changes the selection if the mouse button is down
///
///  @param  event wxMouseEvent & The generated mouse event
///
///  @return void
///
///  @author Derry Bryson @date 04-22-2004
//////////////////////////////////////////////////////////////////////////////
void
NumeReTerminal::OnMouseMove(wxMouseEvent& event)
{
	if (m_selecting)
	{
	    // Get the text coordinates of the mouse
		m_selx2 = event.GetX() / m_charWidth;

		if (m_selx2 >= Width())
			m_selx2 = Width() - 1;

		m_sely2 = event.GetY() / m_charHeight;

		if (m_sely2 >= Height())
			m_sely2 = Height() - 1;

        // Mark the selections
		if (event.AltDown())
			MarkSelection(true);
		else
			MarkSelection();

        // Update the terminal
		// GenericTerminal::Update();
		Refresh();
	}
}


/////////////////////////////////////////////////
/// \brief This member function handles the
/// "Mouse Enter" event.
///
/// \param event wxMouseEvent&
/// \return void
///
/////////////////////////////////////////////////
void NumeReTerminal::OnEnter(wxMouseEvent& event)
{
	if (g_findReplace != nullptr && g_findReplace->IsShown())
	{
		event.Skip();
		return;
	}
	this->SetFocus();
	event.Skip();
}


//////////////////////////////////////////////////////////////////////////////
///  public ClearSelection
///  De-selects all selected text
///
///  @return void
///
///  @author Derry Bryson @date 04-22-2004
//////////////////////////////////////////////////////////////////////////////
void
NumeReTerminal::ClearSelection()
{
	if (!HasSelection() || m_selecting)
		return;

	m_selx1 = m_sely1 = m_selx2 = m_sely2 = 0;
	GetTM()->unselectAll();
    Refresh();
}


//////////////////////////////////////////////////////////////////////////////
///  private MarkSelection
///  Selects the text either in line or in rectangular mode
///
///  @param bRectangular bool
///  @return void
///
///  @author Derry Bryson @date 04-22-2004
//////////////////////////////////////////////////////////////////////////////
void
NumeReTerminal::MarkSelection(bool bRectangular)
{
	int x;
	int y;

	m_marking = true;

	// First deselect all text
	GetTM()->unselectAll();

	// Now select the corresponding region
	if (bRectangular)
	{
	    // Rectangular mode
		for (y = min(m_sely1, m_sely2); y <= max(m_sely1, m_sely2); y++)
		{
			for (x = min(m_selx1, m_selx2); x <= max(m_selx1, m_selx2); x++)
			{
				Select(x, y, 1);
			}
		}
	}
	else
	{
	    // Line mode
	    // More difficult
		if (m_sely1 == m_sely2)
		{
            for (x = min(m_selx1, m_selx2); x <= max(m_selx1, m_selx2); x++)
                Select(x, m_sely1, 1);
		}
		else if (m_sely1 < m_sely2)
		{
			for (x = m_selx1; x < Width(); x++)
				Select(x, m_sely1, 1);

			for (y = m_sely1 + 1; y < m_sely2; y++)
			{
				for (x = 0; x < Width(); x++)
					Select(x, y, 1);
			}

			for (x = 0; x <= m_selx2; x++)
				Select(x, m_sely2, 1);
		}
		else
		{
			for (x = 0; x <= m_selx1; x++)
				Select(x, m_sely1, 1);

			for (y = m_sely2 + 1; y < m_sely1; y++)
			{
				for (x = 0; x < Width(); x++)
					Select(x, y, 1);
			}

			for (x = m_selx2; x < Width(); x++)
				Select(x, m_sely2, 1);
		}
	}

	m_marking = false;
}


//////////////////////////////////////////////////////////////////////////////
///  public HasSelection
///  Checks if any text is selected
///
///  @return bool Whether or not there's any text selected
///
///  @author Derry Bryson @date 04-22-2004
//////////////////////////////////////////////////////////////////////////////
bool
NumeReTerminal::HasSelection()
{
	return (m_selx1 != m_selx2 || m_sely1 != m_sely2);
}


//////////////////////////////////////////////////////////////////////////////
///  public GetSelection
///  Returns the selected text
///
///  @return wxString The selected text
///
///  @author Derry Bryson @date 04-22-2004
//////////////////////////////////////////////////////////////////////////////
wxString
NumeReTerminal::GetSelection()
{
	wxString sel = get_selected_text();

	return sel;
}


//////////////////////////////////////////////////////////////////////////////
///  public virtual DrawText
///  Responsible for actually drawing the terminal text on the widget.  This virtual
///  function is called from GTerm::update_changes.
///
///  @param  fg_color int             The index of the foreground color
///  @param  bg_color int             The index of the background color
///  @param  flags    int             Modifiers for drawing the text
///  @param  x        int             The x position in character cells
///  @param  y        int             The y position in character cells
///  @param  sText    const string&   The string containing the characters to draw
///
///  @return void
///
///  @author Derry Bryson @date 04-22-2004
//////////////////////////////////////////////////////////////////////////////
void
NumeReTerminal::DrawText(int fg_color, int bg_color, int flags, int x, int y, const string& sText)
{
	int
	t;

	// Overwrite the passed colors depending on the flags
	if (flags & SELECTED)
	{
		fg_color = 0;
		bg_color = 15;
	}

	if (flags & INVERSE)
	{
		t = fg_color;
		fg_color = bg_color;
		bg_color = t;
	}

	if (!m_curDC)
		return;

    // Convert std::string into wxString
	wxString
	str(sText);

	// Set the correct font
	if (m_boldStyle != FONT)
	{
		if (flags & UNDERLINE)
			m_curDC->SetFont(m_underlinedFont);
		else
			m_curDC->SetFont(m_normalFont);
	}
	else
	{
		if (flags & BOLD)
		{
			if (flags & UNDERLINE)
				m_curDC->SetFont(m_boldUnderlinedFont);
			else
				m_curDC->SetFont(m_boldFont);
		}
		else
		{
			if (flags & UNDERLINE)
				m_curDC->SetFont(m_underlinedFont);
			else
				m_curDC->SetFont(m_normalFont);
		}
	}

	// Convert x-y char coordinates into pixel coordinates
	x *= m_charWidth;
	y *= m_charHeight;

	// Set colors and background mode
	m_curDC->SetBackgroundMode(wxSOLID);
	m_curDC->SetTextBackground(m_colors[bg_color]);
	m_curDC->SetTextForeground(m_colors[fg_color]);

	// Draw the actual text
	m_curDC->DrawText(str, x, y);
	if (flags & BOLD && m_boldStyle == OVERSTRIKE)
		m_curDC->DrawText(str, x + 1, y);
}


//////////////////////////////////////////////////////////////////////////////
///  private DoDrawCursor
///  Does the actual work of drawing the cursor
///
///  @param  fg_color int            The index of the foreground color
///  @param  bg_color int            The index of the background color
///  @param  flags    int            Modifier flags
///  @param  x        int            The x position of the cursor, in characters
///  @param  y        int            The y position of the cursor, in characters
///  @param  c        unsigned char  The character the cursor is over
///
///  @return void
///
///  @author Derry Bryson @date 04-22-2004
//////////////////////////////////////////////////////////////////////////////
void
NumeReTerminal::DoDrawCursor(int fg_color, int bg_color, int flags, int x, int y, unsigned char c)
{
    // Do nothing, if the terminal is scrolled up
	if (GenericTerminal::IsScrolledUp())
	{
		return;
	}
	int
	t;

	// Overwrite the passed colors depending on the flags
	if (flags & BOLD && m_boldStyle == COLOR)
		fg_color = (fg_color % 8) + 8;

	if (flags & INVERSE)
	{
		t = fg_color;
		fg_color = bg_color;
		bg_color = t;
	}

	if (!m_curDC)
		return;

    // Convert the character into wxString
	wxString
	str((char)c);

	// Set the correct font
	if (m_boldStyle != FONT)
	{
		if (flags & UNDERLINE)
			m_curDC->SetFont(m_underlinedFont);
		else
			m_curDC->SetFont(m_normalFont);
	}
	else
	{
		if (flags & BOLD)
		{
			if (flags & UNDERLINE)
				m_curDC->SetFont(m_boldUnderlinedFont);
			else
				m_curDC->SetFont(m_boldFont);
		}
		else
		{
			if (flags & UNDERLINE)
				m_curDC->SetFont(m_underlinedFont);
			else
				m_curDC->SetFont(m_normalFont);
		}
	}

	// Convert x-y char coordinates into pixel coordinates
	x *= m_charWidth;
	y *= m_charHeight;

	// Set colors and background mode
	m_curDC->SetBackgroundMode(wxSOLID);
	m_curDC->SetTextBackground(m_colors[fg_color]);
	m_curDC->SetTextForeground(m_colors[bg_color]);

	// Draw the actual text
	m_curDC->DrawText(str, x, y);
	if (flags & BOLD && m_boldStyle == OVERSTRIKE)
		m_curDC->DrawText(str, x + 1, y);
}


//////////////////////////////////////////////////////////////////////////////
///  public virtual DrawCursor
///  Draws the cursor on the terminal widget.  This virtual function is called
///  from GTerm::update_changes.
///
///  @param  fg_color int            The index of the foreground color
///  @param  bg_color int            The index of the background color
///  @param  flags    int            Modifiers for drawing the cursor
///  @param  x        int            The x position in character cells
///  @param  y        int            The y position in character cells
///  @param  c        unsigned char  The character that underlies the cursor
///
///  @return void
///
///  @author Derry Bryson @date 04-22-2004
//////////////////////////////////////////////////////////////////////////////
void
NumeReTerminal::DrawCursor(int fg_color, int bg_color, int flags, int x, int y, unsigned char c)
{
    // Set cursor-related member variables
	m_curX = x;
	m_curY = y;
	m_curFG = fg_color;
	m_curBG = bg_color,
	m_curFlags = flags;
	m_curChar = c;

	// Stop the timer
	if (m_timer.IsRunning())
		m_timer.Stop();

    // Draw the cursor at its position
	DoDrawCursor(fg_color, bg_color, flags, x, y, c);

	// Restart the timer
	if (m_curBlinkRate)
	{
		m_timer.Start(m_curBlinkRate);
		m_curState = 1;
	}
}


//////////////////////////////////////////////////////////////////////////////
///  private OnTimer
///  Blinks the cursor each time it goes off
///
///  @param  event wxTimerEvent & The generated timer event
///
///  @return void
///
///  @author Derry Bryson @date 04-22-2004
//////////////////////////////////////////////////////////////////////////////
void
NumeReTerminal::OnTimer(wxTimerEvent& WXUNUSED(event))
{
	wxClientDC* dc = nullptr;

	if (m_init)
		return;

    // Do nothing if the cursor coordinates are invalid
	if (m_curX == -1 || m_curY == -1 || m_selecting)
		return;

    // Do nothing, if the cursor is invisible
	if (GetMode() & CURSORINVISIBLE)
	{
		//wxLogDebug("Skipping cursor");
		return;
	}
	//wxLogDebug("Drawing cursor");
	if (!m_curDC)
	{
		dc = new wxClientDC(this);
		m_curDC = dc;
	}

	// Draw the blinking cursor
	// Blinking is done by alternating fore and background colors
	if (m_curBlinkRate)
	{
		m_curState++;
		if (m_curState & 1 && m_curX != -1 && m_curY != -1)
			DoDrawCursor(m_curBG, m_curFG, m_curFlags, m_curX, m_curY, m_curChar);
		else
			DoDrawCursor(m_curFG, m_curBG, m_curFlags, m_curX, m_curY, m_curChar);
	}

	if (dc)
	{
		delete dc;
		m_curDC = nullptr;
	}
}


//////////////////////////////////////////////////////////////////////////////
///  public virtual ClearChars
///  Clears a section of characters from the screen.  This virtual function
///  is called from GTerm::update_changes.
///
///  @param  bg_color int  The background color to replace the characters with
///  @param  x        int  The starting x position, in characters
///  @param  y        int  The starting y position, in characters
///  @param  w        int  The width of the area to be cleared, in characters
///  @param  h        int  The height of the area to be cleared, in characters
///
///  @return void
///
///  @author Derry Bryson @date 04-22-2004
//////////////////////////////////////////////////////////////////////////////
void
NumeReTerminal::ClearChars(int bg_color, int x, int y, int w, int h)
{
	x *= m_charWidth;
	y *= m_charHeight;
	w *= m_charWidth;
	h *= m_charHeight;

	if (!m_curDC)
		return;

	// Clear the area by drawing a rectangle with the background color
	m_curDC->SetPen(m_colorPens[bg_color]);
	m_curDC->SetBrush(wxBrush(m_colors[bg_color], wxSOLID));
	m_curDC->DrawRectangle(x, y, w /* + 1*/, h /*+ 1*/);
}


//////////////////////////////////////////////////////////////////////////////
///  public virtual ModeChange
///  Changes the drawing mode between VT100 and PC
///
///  @param  state int  The new state
///
///  @return void
///
///  @author Derry Bryson @date 04-22-2004
//////////////////////////////////////////////////////////////////////////////
void
NumeReTerminal::ModeChange(int state)
{
	ClearSelection();

	m_colors = m_color_defs;
	m_colorPens = m_colorPen_defs;

	GenericTerminal::ModeChange(state);
}


//////////////////////////////////////////////////////////////////////////////
///  public virtual Bell
///  Rings the system bell
///
///  @return void
///
///  @author Derry Bryson @date 04-22-2004
//////////////////////////////////////////////////////////////////////////////
void
NumeReTerminal::Bell()
{
#ifdef __WIN32__
	WinMessageBeep();
#else
	wxBell();
#endif
}


//////////////////////////////////////////////////////////////////////////////
///  public UpdateSize
///  Updates the terminal's size in characters after it has been resized on the screen.
///
///  @return void
///
///  @author Mark Erikson @date 04-22-2004
//////////////////////////////////////////////////////////////////////////////
void NumeReTerminal::UpdateSize()
{
	// prevent any nasty recursion
	if (m_inUpdateSize)
	{
		return;
	}

	m_inUpdateSize = true;
	int charWidth, charHeight, w, h;
	wxClientDC* dc = nullptr;

	if (!m_curDC)
	{
        dc = new wxClientDC(this);
		m_curDC = dc;
	}
	else
		return;

	// Set the normal font
	dc->SetFont(m_boldFont);

	// Calculate the correct text extent
	dc->GetTextExtent("M", &charWidth, &h); // EKHL: Changed because Heigth made no sense
	dc->GetTextExtent("My", &w, &charHeight);

	wxSize currentClientSize = GetClientSize();

	// Get the number of characters per line and the number of lines
	int numCharsInLine = currentClientSize.GetX() / charWidth;
	int numLinesShown = currentClientSize.GetY() / charHeight;

    // Update the size, if the calculated numbers are not matching the previous ones
	if ( (numCharsInLine != m_charsInLine) || (numLinesShown != m_linesDisplayed))
	{
		wxString message;

		// FINALLY!  Finally killed the memory leak!  The problem is that somehow a size event
		// was generating negative numbers for these values, which led to weird things happening.
		if ( (numCharsInLine > 0) && (numLinesShown > 0))
		{
			m_charsInLine = numCharsInLine;
			m_linesDisplayed = numLinesShown;
			// tell the GTerm core to resize itself
			ResizeTerminal(numCharsInLine, numLinesShown);
			{
				wxCriticalSectionLocker lock(m_kernelCS);
				_kernel.updateLineLenght(numCharsInLine);
			}
		}
	}

	m_inUpdateSize = false;
	Refresh();

	delete dc;
	m_curDC = nullptr;
}


/////////////////////////////////////////////////
/// \brief This member function sets the new
/// colors to the internal pen definitions.
///
/// \return void
///
/////////////////////////////////////////////////
void NumeReTerminal::UpdateColors()
{
	GetDefColors(m_color_defs);

	m_colors = m_color_defs;

	SetBackgroundColour(m_colors[0]);
	m_parent->SetBackgroundColour(m_colors[0]);

	for (int i = 0; i < 16; i++)
		m_colorPen_defs[i] = wxPen(m_color_defs[i], 1, wxSOLID);

	m_colorPens = m_colorPen_defs;
	update_changes();
	wxWindow::Update();
	m_parent->Update();
}


//////////////////////////////////////////////////////////////////////////////
///  public virtual ResizeTerminal
///  <Resizes the terminal to a given number of characters high and wide
///
///  @param  width    int  The new number of characters wide
///  @param  height    int  The new number of characters high
///
///  @return void
///
///  @author Derry Bryson @date 04-22-2004
//////////////////////////////////////////////////////////////////////////////
void
NumeReTerminal::ResizeTerminal(int width, int height)
{
	int
	w,
	h;

	ClearSelection();

	/*
	**  Determine window size from current font
	*/
	wxClientDC
	dc(this);

	// Calualte the correct text extents
	dc.SetFont(m_boldFont);
	dc.GetTextExtent("M", &m_charWidth, &h); // EKHL: Changed because Heigth made no sense
	dc.GetTextExtent("My", &w, &m_charHeight);

	w = width * m_charWidth;
	h = height * m_charHeight;

	/*
	**  Create our bitmap for copying
	*/
	if (m_bitmap)
	{
		m_memDC.SelectObject(wxNullBitmap);
		delete m_bitmap;
	}
	m_bitmap = new wxBitmap(w, h);
	m_memDC.SelectObject(*m_bitmap);

	/*
	**  Set window size
	*/
	SetSize(w, h);

	/*
	**  Set terminal size
	*/
	GenericTerminal::ResizeTerminal(width, height);

	m_width = width;
	m_height = height;

	/*
	**  Send event
	*/
	if (!m_init)
	{
		wxCommandEvent e(wxEVT_COMMAND_TERM_RESIZE, GetId());
		e.SetEventObject(this);
		GetParent()->GetEventHandler()->ProcessEvent(e);
	}
}


/** \brief Processes text received from the keybord or clipboard
 *
 * \param len int
 * \param sData const string&
 * \return void
 *
 */
void NumeReTerminal::ProcessInput(int len, const string& sData)
{
	scrollToInput();

	// Delete selected characters and/or remove
	// the selection
    if (HasSelection())
    {
        delSelected();
        ClearSelection();
    }


	wxClientDC
	dc(this);

	m_curDC = &dc;
	GenericTerminal::ProcessInput(len, sData);
	m_curDC = nullptr;
}


/** \brief Processes text received from the kernel
 *
 * \param len int
 * \param sData const string&
 * \return void
 *
 */
void NumeReTerminal::ProcessOutput(int len, const string& sData)
{
	if (HasSelection())
		ClearSelection();
	wxClientDC
	dc(this);
	m_curDC = &dc;
	GenericTerminal::ProcessOutput(len, sData);
	m_curDC = nullptr;
}


//////////////////////////////////////////////////////////////////////////////
///  private OnActivate
///  Sets the terminal's active state - determines whether or not to draw the cursor
///
///  @param  event wxActivateEvent & The generated activate event
///
///  @return void
///
///  @author Mark Erikson @date 04-22-2004
//////////////////////////////////////////////////////////////////////////////
void NumeReTerminal::OnActivate(wxActivateEvent& event)
{
	m_isActive = event.GetActive();
}


//////////////////////////////////////////////////////////////////////////////
///  private OnGainFocus
///  Enables the cursor
///
///  @param  event wxFocusEvent & The generated focus event
///
///  @return void
///
///  @author Mark Erikson @date 04-22-2004
//////////////////////////////////////////////////////////////////////////////
void NumeReTerminal::OnGainFocus(wxFocusEvent& event)
{
	this->clear_mode_flag(CURSORINVISIBLE);
	GenericTerminal::Update();
}


//////////////////////////////////////////////////////////////////////////////
///  private OnLoseFocus
///  Disables the cursor
///
///  @param  event wxFocusEvent & The generated focus event
///
///  @return void
///
///  @author Mark Erikson @date 04-22-2004
//////////////////////////////////////////////////////////////////////////////
void NumeReTerminal::OnLoseFocus(wxFocusEvent& event)
{
	this->set_mode_flag(CURSORINVISIBLE);
	GenericTerminal::Update();
}


/////////////////////////////////////////////////
/// \brief Inform the GUI that the kernel is
/// currently busy.
///
/// \return void
///
/////////////////////////////////////////////////
void NumeReTerminal::Busy()
{
    m_isBusy = true;
    m_wxParent->Busy();
}


/////////////////////////////////////////////////
/// \brief Inform the GUI that the kernel is
/// ready for calculation.
///
/// \return void
///
/////////////////////////////////////////////////
void NumeReTerminal::Ready()
{
    m_isBusy = true;
    m_wxParent->Ready();
}


//////////////////////////////////////////////////////////////////////////////
///  public ScrollTerminal
///  Scrolls the terminal text
///
///  @param  numLines int   The number of lines to scroll
///  @param  scrollUp bool  [=true] True to scroll up, false to scroll down
///
///  @return void
///
///  @author Mark Erikson @date 04-22-2004
//////////////////////////////////////////////////////////////////////////////
void NumeReTerminal::ScrollTerminal(int numLines, bool scrollUp /* = true */)
{
	if (GenericTerminal::Scroll(numLines, scrollUp))
		Refresh();
}


//////////////////////////////////////////////////////////////////////////////
///  private OnSize
///  Lets the terminal resize the text whenever the window is resized
///
///  @param  event wxSizeEvent & The generated size event
///
///  @return void
///
///  @author Mark Erikson @date 04-22-2004
//////////////////////////////////////////////////////////////////////////////
void NumeReTerminal::OnSize(wxSizeEvent& event)
{
	UpdateSize();
}


/////////////////////////////////////////////////
/// \brief Fallback for the virtual definition.
///
/// \param width int
/// \param height int
/// \return void
///
/////////////////////////////////////////////////
void NumeReTerminal::UpdateRemoteSize(int width, int height)
{
}

