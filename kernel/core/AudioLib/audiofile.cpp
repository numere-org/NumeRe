/*****************************************************************************
    NumeRe: Framework fuer Numerische Rechnungen
    Copyright (C) 2021  Erik Haenel et al.

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

#include "audiofile.hpp"

#include "wavfile.hpp"

namespace Audio
{
    /////////////////////////////////////////////////
    /// \brief Return a audio file type depending on
    /// the file extension or a nullptr if the file
    /// type is not supported.
    ///
    /// \param sFileName const std::string&
    /// \return File*
    ///
    /////////////////////////////////////////////////
    File* getAudioFileByType(const std::string& sFileName)
    {
        std::string sExt = sFileName.substr(sFileName.rfind('.'));

        if (sExt == ".wav")
            return new WavFile(sFileName);

        return nullptr;
    }
}


