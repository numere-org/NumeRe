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

#ifndef WAVFILE_HPP
#define WAVFILE_HPP

#include "audiofile.hpp"
#include <fstream>

namespace Audio
{
    /////////////////////////////////////////////////
    /// \brief This structure represents the central
    /// part of the wave file header in a format,
    /// which can be directly written to and read
    /// from the wave file.
    /////////////////////////////////////////////////
    struct WavFileHeader
    {
        uint16_t formatTag;
        uint16_t channels;
        uint32_t sampleRate;
        uint32_t bytesPerSecond;
        uint16_t blockAlign;
        uint16_t bitsPerSample;

        WavFileHeader() : formatTag(1), channels(1), sampleRate(44100), bytesPerSecond(44100 * 2), blockAlign(2), bitsPerSample(16) {}
    };


    /////////////////////////////////////////////////
    /// \brief This class implements the wave file
    /// type using PCM encoding (the simplest
    /// encoding). Due to its simplicity it is a
    /// seekable file type.
    /////////////////////////////////////////////////
    class WavFile : public SeekableFile
    {
        private:
            std::string sName;
            mutable std::fstream m_WavFileStream;
            WavFileHeader m_Header;
            uint32_t m_DataBlockLength;
            const long long int m_StreamOffset = 44;
            bool isNewFile;

            bool readHeader();
            void closeFile();

        public:
            WavFile(const std::string& sFileName);
            virtual ~WavFile();

            /////////////////////////////////////////////////
            /// \brief Returns, whether the currently opened
            /// file is actually open and readable.
            ///
            /// \return virtual bool
            ///
            /////////////////////////////////////////////////
            virtual bool isValid() const
            {
                return m_WavFileStream.is_open() && m_WavFileStream.good();
            }

            virtual void newFile() override;

            /////////////////////////////////////////////////
            /// \brief Set the channels, which shall be used
            /// in the new file.
            ///
            /// \param channels size_t
            /// \return virtual void
            ///
            /////////////////////////////////////////////////
            virtual void setChannels(size_t channels) override
            {
                m_Header.channels = channels;
            }

            /////////////////////////////////////////////////
            /// \brief Set the sample rate, which shall be
            /// used in the new file.
            ///
            /// \param freq size_t
            /// \return virtual void
            ///
            /////////////////////////////////////////////////
            virtual void setSampleRate(size_t freq) override
            {
                m_Header.sampleRate = freq;
            }

            virtual void write(const Sample& sample) override;

            /////////////////////////////////////////////////
            /// \brief Get the number of channels of the
            /// current file.
            ///
            /// \return virtual size_t
            ///
            /////////////////////////////////////////////////
            virtual size_t getChannels() const override
            {
                return m_Header.channels;
            }

            /////////////////////////////////////////////////
            /// \brief Get the sample rate of the current
            /// file.
            ///
            /// \return virtual size_t
            ///
            /////////////////////////////////////////////////
            virtual size_t getSampleRate() const override
            {
                return m_Header.sampleRate;
            }

            /////////////////////////////////////////////////
            /// \brief Get the length in samples of the
            /// current file.
            ///
            /// \return virtual size_t
            ///
            /////////////////////////////////////////////////
            virtual size_t getLength() const override
            {
                return m_DataBlockLength / (m_Header.channels * m_Header.bitsPerSample/8);
            }

            virtual Sample read() const override;

            /////////////////////////////////////////////////
            /// \brief Get the current position in samples of
            /// the current file.
            ///
            /// \return virtual size_t
            ///
            /////////////////////////////////////////////////
            virtual size_t getPosition() const override
            {
                if (m_WavFileStream.good())
                    return (m_WavFileStream.tellg() - m_StreamOffset) / (m_Header.channels * m_Header.bitsPerSample/8);

                return 0;
            }

            /////////////////////////////////////////////////
            /// \brief Set the current position in Samples in
            /// the current file.
            ///
            /// \param pos size_t
            /// \return virtual void
            ///
            /////////////////////////////////////////////////
            virtual void setPosition(size_t pos) override
            {
                if (m_WavFileStream.good() && pos * (m_Header.channels * m_Header.bitsPerSample/8) < m_DataBlockLength)
                    m_WavFileStream.seekg(pos * (m_Header.channels * m_Header.bitsPerSample/8) + m_StreamOffset);
            }

            virtual std::vector<Sample> readSome(size_t len) const override;
            virtual void writeSome(const std::vector<Sample> vSamples) override;
    };
}

#endif // WAVFILE_HPP

