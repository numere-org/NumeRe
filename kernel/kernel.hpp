#include <string>

using namespace std;

class wxTerm;

class NumeReKernel
{
    private:
        bool bCancelSignal;
        string sCommandLine;
        string sAnswer;
        //datasets
        //mu::Parser _parser;
        //Datafile _data;
        //Cache _cache;
        //Settings _option;
        //Define _functions;
        //mglGraph _graph;
    public:
        static wxTerm* m_parent;
        // return values indicating status:
        enum KernelStatus
        {
            NUMERE_QUIT = -1,
            NUMERE_ERROR = 0,
            NUMERE_DONE = 1,
            NUMERE_PRINTLINE,
            NUMERE_PENDING,
            NUMERE_CALC_UPDATE,
            NUMERE_STATUSBAR_UPDATE
        };

        NumeReKernel();
        ~NumeReKernel();
        string ReadAnswer();
        void StartUp(wxTerm* _parent);
        KernelStatus MainLoop(const string& sCommand);
        void CloseSession();
        void CancelCalculation()
        {
            bCancelSignal = true;
        }

        static void print(const string& sLine);
        static void statusBar(int nStep, int nFirstStep, int nFinalStep, const string& sType);
};
