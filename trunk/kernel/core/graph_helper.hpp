/*****************************************************************************
    NumeRe: Framework fuer Numerische Rechnungen
    Copyright (C) 2014  Erik Haenel et al.

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

#ifndef GRAPH_HELPER_HPP
#define GRAPH_HELPER_HPP

#include <mgl2/wnd.h>
#include <mgl2/mgl.h>
#include <string>
#include <vector>

#include "plotdata.hpp"
#include "datafile.hpp"
#include "define.hpp"
#include "settings.hpp"
#include "ParserLib/muParser.h"
#include "tools.hpp"
#include "error.hpp"

bool parser_ExprNotEmpty(const string&);
int parser_StringParser(string&, string&, Datafile&, Parser&, const Settings&, bool);
string parser_Prompt(const string&);
int parser_SplitArgs(string&, string&, const char&, const Settings&, bool);
void parser_CheckIndices(int&, int&);
void parser_CheckIndices(long long int&, long long int&);
void parser_VectorToExpr(string&, const Settings&);
// --> Integration_Vars-Structure: Name, Werte und Grenzen der Integrationsvariablen. Erlaubt bis zu 3D-Integration <--

struct Integration_Vars
{
    string sName[4] = {"x", "y", "z", "t"};
    value_type vValue[4][4];
};

extern Integration_Vars parser_iVars;
extern mglGraph _fontData;
mglPoint parser_CalcCutBox(double, double dRanges[3][2], int, int, bool);
double parser_getProjBackground(double, double dRanges[3][2], int);



using namespace std;
using namespace mu;

class Graph_helper : public mglDraw
{
    private:
        Datafile* _data;
        Parser* _parser;
        Settings* _option;
        Define* _functions;
        PlotData* _pData;

        mglData** _mDataPlots;
        int* nDataDim;
        int nDataPlots;
        string __sCmd;

    public:
        Graph_helper(string& sCommand, Datafile* __data, Parser* __parser, Settings* __option, Define* __functions, PlotData* __pData);
        ~Graph_helper();

        int Draw(mglGraph* _graph);
        void Reload();
};


#endif // GRAPH_HELPER_HPP

