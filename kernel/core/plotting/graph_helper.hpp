/*****************************************************************************
    NumeRe: Framework fuer Numerische Rechnungen
    Copyright (C) 2017  Erik Haenel et al.

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

#include "plotdata.hpp"

using namespace std;

/////////////////////////////////////////////////
/// \brief This class encapsulates the mglGraph
/// object during transmission from the kernel to
/// the GUI.
/////////////////////////////////////////////////
class GraphHelper : public mglDraw
{
    private:
        mglGraph* _graph;
        bool bAlphaActive;
        bool bLightActive;
        bool bHires;
        double dAspect;
        string sTitle;

    public:
        GraphHelper(mglGraph* __graph, const PlotData& _pData);
        ~GraphHelper();

        int Draw(mglGraph* _graph);
        void Reload();
        mglGraph* setGrapher()
            {return _graph;}
        bool getLighting()
            {return bLightActive;}
        bool getAlpha()
            {return bAlphaActive;}
        bool getHires()
            {return bHires;}
        double getAspect()
            {return dAspect;}
        void setAspect(double aspect)
            {dAspect = aspect;}
        string getTitle()
            {
                if (sTitle.length())
                    return sTitle;
                return "Graph";
            }
};


#endif // GRAPH_HELPER_HPP

