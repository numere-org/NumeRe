#include "compilerevent.h"


DEFINE_EVENT_TYPE(chEVT_COMPILER_START)
DEFINE_EVENT_TYPE(chEVT_COMPILER_PROBLEM)
DEFINE_EVENT_TYPE(chEVT_COMPILER_END)

CompilerEvent::CompilerEvent(wxEventType t)
	: wxEvent(0, t)
{
}
