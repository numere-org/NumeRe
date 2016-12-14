//////////////////////////////////////////////////////////////////////
//
//   ChameleonProcessEvent
//
//////////////////////////////////////////////////////////////////////

#ifndef __CHAMELEON_PROCESS_EVENT__H__
#define __CHAMELEON_PROCESS_EVENT__H__

#include <wx/process.h>
#include <wx/event.h>

// -------------------------------------------------------------------
// ProcessEvent
// -------------------------------------------------------------------
DECLARE_EVENT_TYPE(chEVT_PROCESS_ENDED, wxID_ANY)
DECLARE_EVENT_TYPE(chEVT_PROCESS_STDOUT, wxID_ANY)
DECLARE_EVENT_TYPE(chEVT_PROCESS_STDERR, wxID_ANY)


class ChameleonProcessEvent : public wxEvent
{
	public:
		ChameleonProcessEvent(wxEventType eventtype);
		ChameleonProcessEvent(const ChameleonProcessEvent& event);

		void SetInt(int i) {m_int = i; }
		void SetPid(int i) {m_pid = i; }
		void SetString(wxString s) { m_string = s; }

		int GetInt() const { return m_int; }
		int GetPid() const
		{ 
			int pid = m_pid;
			return m_pid; 
		}
		wxString GetString() const { return m_string; }

		virtual wxEvent *Clone() const { return new ChameleonProcessEvent(*this); }

	private:
		//int m_type;
		int m_int;
		wxString m_string;
		int m_pid;

};

typedef void (wxEvtHandler::*ChameleonProcessEventFunction)(ChameleonProcessEvent&);

#define EVT_PROCESS_ENDED(fn) \
	DECLARE_EVENT_TABLE_ENTRY( \
			chEVT_PROCESS_ENDED, wxID_ANY, wxID_ANY, \
			(wxObjectEventFunction)(wxEventFunction)(ChameleonProcessEventFunction)&fn, \
			(wxObject *) NULL),

#define EVT_PROCESS_STDOUT(fn) \
	DECLARE_EVENT_TABLE_ENTRY( \
			chEVT_PROCESS_STDOUT, wxID_ANY, wxID_ANY, \
			(wxObjectEventFunction)(wxEventFunction)(ChameleonProcessEventFunction)&fn, \
			(wxObject *) NULL),

#define EVT_PROCESS_STDERR(fn) \
	DECLARE_EVENT_TABLE_ENTRY( \
			chEVT_PROCESS_STDERR, wxID_ANY, wxID_ANY, \
			(wxObjectEventFunction)(wxEventFunction)(ChameleonProcessEventFunction)&fn, \
			(wxObject *) NULL),


#endif // __CHAMELEON_PROCESS_EVENT__H__