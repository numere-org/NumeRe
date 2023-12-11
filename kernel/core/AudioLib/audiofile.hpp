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

#ifndef AUDIOFILE_HPP
#define AUDIOFILE_HPP

#include <vector>
#include <string>
#include <cmath>

namespace Audio
{
    /////////////////////////////////////////////////
    /// \brief Defines a sample in the audio stream,
    /// i.e. a single value per channel.
    /////////////////////////////////////////////////
    struct Sample
    {
        float leftOrMono;
        float right;

        Sample(float _mono) : leftOrMono(_mono), right(NAN) {}
        Sample(float _left, float _right) : leftOrMono(_left), right(_right) {}
    };


    /////////////////////////////////////////////////
    /// \brief This class represents a generic audio
    /// file with reading and writing functionalities.
    /////////////////////////////////////////////////
    class File
    {
        public:
            File() {}
            virtual ~File() {}

            virtual bool isValid() const = 0;

            virtual void newFile() = 0;
            virtual void setChannels(size_t channels) = 0;
            virtual void setSampleRate(size_t freq) = 0;
            virtual void write(const Sample& frame) = 0;

            virtual size_t getChannels() const = 0;
            virtual size_t getSampleRate() const = 0;
            virtual size_t getLength() const = 0;
            virtual Sample read() const = 0;

            /////////////////////////////////////////////////
            /// \brief Audio files, which inherit from this
            /// class do not have any seeking functionality.
            ///
            /// \return virtual bool
            ///
            /////////////////////////////////////////////////
            virtual bool isSeekable() const
            {
                return false;
            }
    };


    /////////////////////////////////////////////////
    /// \brief This class extends the generic audio
    /// file with seeking functionalities as well as
    /// possibilities for reading and writing whole
    /// blocks of the file.
    /////////////////////////////////////////////////
    class SeekableFile : public File
    {
        public:
            SeekableFile() {}

            /////////////////////////////////////////////////
            /// \brief Overrides the base classes member
            /// function to signal the possibility to safely
            /// up-cast to this class.
            ///
            /// \return virtual bool
            ///
            /////////////////////////////////////////////////
            virtual bool isSeekable() const override
            {
                return true;
            }

            virtual size_t getPosition() const = 0;
            virtual void setPosition(size_t pos) = 0;

            virtual std::vector<Sample> readSome(size_t len) const = 0;
            virtual void writeSome(const std::vector<Sample> vFrames) = 0;
    };


    File* getAudioFileByType(const std::string& sFileName);
}

#endif // AUDIOFILE_HPP



