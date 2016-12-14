//#include "kernel.hpp"
//#include <windows.h>
#include "wx/wx.h"
#include "../gui/wxterm.h"

wxTerm* NumeReKernel::m_parent = nullptr;

NumeReKernel::NumeReKernel()
{
    //RecievedCommand = false;
    sCommandLine.clear();
    sAnswer.clear();
    bCancelSignal = false;

    //StartUp();
}

NumeReKernel::~NumeReKernel()
{
    CloseSession();
}

void NumeReKernel::StartUp(wxTerm* _parent)
{
    if (_parent && m_parent == nullptr)
        m_parent = _parent;
    //Do some start-up stuff here
}

NumeReKernel::KernelStatus NumeReKernel::MainLoop(const string& sCommand)
{
    if (!m_parent)
        return NUMERE_ERROR;
    sCommandLine += sCommand;
    if (!sCommandLine.length())
        return NUMERE_PENDING;
    while (sCommandLine.front() == ' ')
        sCommandLine.erase(0,1);
    if (!sCommandLine.length())
        return NUMERE_PENDING;
    while (sCommandLine.back() == ' ')
        sCommandLine.pop_back();
    if (sCommandLine.substr(sCommandLine.length()-2,2) == "\\\\")
    {
        sCommandLine.erase(sCommandLine.length()-2);
        return NUMERE_PENDING;
    }


    NumeReKernel::print("Das ist ein test");
    //Sleep(2000);

    if (sCommandLine == "quit")
        return NumeReKernel::NUMERE_QUIT;
    sAnswer = "ans = " + sCommandLine;
    sCommandLine.clear();
    //RecievedCommand = false;
    return NUMERE_DONE;
}

void NumeReKernel::CloseSession()
{
    //Do some clean-up stuff here
    sCommandLine.clear();
    sAnswer.clear();
    m_parent = nullptr;
}

string NumeReKernel::ReadAnswer()
{
    return sAnswer;
}

// This is the virtual cout function. The port from the kernel of course needs some tweaking
void NumeReKernel::print(const string& sLine)
{
    if (!m_parent)
        return;
    else
    {
        wxCriticalSectionLocker lock(m_parent->m_kernelCS);
        m_parent->m_sAnswer = sLine;
        if (m_parent->m_KernelStatus != NumeReKernel::NUMERE_PRINTLINE && m_parent->m_KernelStatus != NumeReKernel::NUMERE_CALC_UPDATE)
            m_parent->m_KernelStatus = NumeReKernel::NUMERE_PRINTLINE;
    }
    wxQueueEvent(m_parent->GetEventHandler(), new wxThreadEvent());
    Sleep(10);
}

// This shall replace the corresponding function from "tools.hpp"
void NumeReKernel::statusBar(int nStep, int nFirstStep, int nFinalStep, const string& sType)
{};






