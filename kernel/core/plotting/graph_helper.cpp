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

#include "graph_helper.hpp"
#include "../../kernel.hpp"
#include "../utils/tools.hpp"

/////////////////////////////////////////////////
/// \brief Constructor.
///
/// \param __graph mglGraph*
/// \param _pData const PlotData&
///
/////////////////////////////////////////////////
GraphHelper::GraphHelper(mglGraph* __graph, const PlotData& _pData)
{
    _graph = __graph;
    bAlphaActive = _pData.getSettings(PlotData::LOG_ALPHA);
    bLightActive = _pData.getSettings(PlotData::INT_LIGHTING);
    bHires = _pData.getSettings(PlotData::INT_HIGHRESLEVEL) == 2;
    dAspect = _pData.getSettings(PlotData::FLOAT_ASPECT);
    sTitle = _pData.getSettings(PlotData::STR_COMPOSEDTITLE);
}


/////////////////////////////////////////////////
/// \brief Destructor. Deletes the internal
/// mglGraph pointer.
/////////////////////////////////////////////////
GraphHelper::~GraphHelper()
{
    if (_graph)
        delete _graph;
}


/////////////////////////////////////////////////
/// \brief Empty virtual function implementation.
///
/// \param _graph mglGraph*
/// \return int
///
/////////////////////////////////////////////////
int GraphHelper::Draw(mglGraph* _graph)
{
    return 0;
}


/////////////////////////////////////////////////
/// \brief Empty virtual function implementation.
///
/// \return void
///
/////////////////////////////////////////////////
void GraphHelper::Reload()
{
    return;
}

