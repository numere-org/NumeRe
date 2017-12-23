#include "chameleonprocessevent.h"
#include "../common/debug.h"

#ifdef _DEBUG
	#define new DEBUG_NEW
#endif

DEFINE_EVENT_TYPE(chEVT_PROCESS_ENDED)
DEFINE_EVENT_TYPE(chEVT_PROCESS_STDOUT)
DEFINE_EVENT_TYPE(chEVT_PROCESS_STDERR)

ChameleonProcessEvent::ChameleonProcessEvent(wxEventType eventtype)
	: wxEvent(0, eventtype)
{
	//
}

ChameleonProcessEvent::ChameleonProcessEvent(const ChameleonProcessEvent& otherEvent)
: wxEvent(otherEvent)
{
	this->SetString(otherEvent.GetString());
	this->SetInt(otherEvent.GetInt());
	this->SetPid(otherEvent.GetPid());

}