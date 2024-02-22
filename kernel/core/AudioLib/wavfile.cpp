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

#include "wavfile.hpp"
#include <cstring>

#define PCM 0x0001

namespace Audio
{
    /////////////////////////////////////////////////
    /// \brief Applies the necessary conversion to
    /// obtain a signed floating point value from an
    /// unsigned integer.
    ///
    /// \param val float
    /// \return float
    ///
    /////////////////////////////////////////////////
    static float convertFromUnsigned(float val)
    {
        return val > 1.0 ? val-2.0 : val;
    }


    /////////////////////////////////////////////////
    /// \brief Calculates the maximal value available
    /// with the current bitdepth.
    ///
    /// \param bitdepth uint16_t
    /// \return float
    ///
    /////////////////////////////////////////////////
    static float getMaxVal(uint16_t bitdepth)
    {
        return (1 << (bitdepth-1)) - 1;
    }

    /////////////////////////////////////////////////
    /// \brief Constructor. Tries to open the wave
    /// file, if it exists.
    ///
    /// \param sFileName const std::string&
    ///
    /////////////////////////////////////////////////
    WavFile::WavFile(const std::string& sFileName)
    {
        m_WavFileStream.open(sFileName.c_str(), std::ios_base::in | std::ios_base::binary);
        sName = sFileName;

        if (!readHeader())
            m_WavFileStream.close();
    }


    /////////////////////////////////////////////////
    /// \brief Destructor. Will close the file stream
    /// and update the header.
    /////////////////////////////////////////////////
    WavFile::~WavFile()
    {
        closeFile();
    }


    /////////////////////////////////////////////////
    /// \brief Private member function to read the
    /// wave file header to memory. It will
    /// ensure that the current file is actually a
    /// RIFF WAVE file.
    ///
    /// \return bool
    ///
    /////////////////////////////////////////////////
    bool WavFile::readHeader()
    {
        if (!m_WavFileStream.good())
        {
            m_DataBlockLength = 0;
            return false;
        }
        else
        {
            char headerInfo[5] = {0,0,0,0,0};
            m_WavFileStream.read(headerInfo, 4);

            if (strcmp(headerInfo, "RIFF"))
            {
                m_DataBlockLength = 0;
                return false;
            }

            m_WavFileStream.read(headerInfo, 4);
            m_WavFileStream.read(headerInfo, 4);

            if (strcmp(headerInfo, "WAVE"))
            {
                m_DataBlockLength = 0;
                return false;
            }

            m_WavFileStream.seekg(20);
            m_WavFileStream.read((char*)&m_Header, sizeof(WavFileHeader));

            if (m_Header.formatTag != PCM)
            {
                m_DataBlockLength = 0;
                return false;
            }

            m_WavFileStream.seekg(40);
            m_WavFileStream.read((char*)&m_DataBlockLength, 4);

            m_maxVal = getMaxVal(m_Header.bitsPerSample);
        }

        return true;
    }


    /////////////////////////////////////////////////
    /// \brief Updates the wave file header, which is
    /// currently open, and closes the file afterwards.
    ///
    /// \return void
    ///
    /////////////////////////////////////////////////
    void WavFile::closeFile()
    {
        if (m_WavFileStream.is_open())
        {
            m_WavFileStream.seekp(0, std::ios_base::end);
            uint32_t pos = m_WavFileStream.tellp()-8LL;
            m_WavFileStream.seekp(4);
            m_WavFileStream.write((char*)&pos, 4);
            m_WavFileStream.seekp(40);
            pos -= 36;
            m_WavFileStream.write((char*)&pos, 4);
            m_WavFileStream.close();
        }
    }


    /////////////////////////////////////////////////
    /// \brief Prepares the wave file header for a
    /// new file, opens the stream and writes the
    /// header to the file. Set number of channels
    /// and sample frequency before calling this
    /// member function.
    ///
    /// \return void
    ///
    /////////////////////////////////////////////////
    void WavFile::newFile()
    {
        closeFile();

        // prepare header
        m_Header.formatTag = PCM;
        m_Header.bitsPerSample = 16;
        m_Header.bytesPerSecond = m_Header.sampleRate * m_Header.bitsPerSample / 8 * m_Header.channels;
        m_Header.blockAlign = m_Header.bitsPerSample / 8 * m_Header.channels;
        m_maxVal = getMaxVal(m_Header.bitsPerSample);

        m_WavFileStream.open(sName.c_str(), std::ios_base::out | std::ios_base::binary | std::ios_base::trunc);
        m_WavFileStream.write("RIFF    WAVEfmt ", 16);
        uint32_t nSize = 16;
        m_WavFileStream.write((char*)&nSize, 4);
        m_WavFileStream.write((char*)&m_Header, sizeof(WavFileHeader));
        m_WavFileStream.write("data    ", 8);
    }


    /////////////////////////////////////////////////
    /// \brief Write a sample to the audio stream.
    ///
    /// \param sample const Sample&
    /// \return void
    ///
    /////////////////////////////////////////////////
    void WavFile::write(const Sample& sample)
    {
        if (m_WavFileStream.is_open() && m_WavFileStream.good())
        {
            int16_t channelvalue = sample.leftOrMono * m_maxVal;
            m_WavFileStream.write((char*)&channelvalue, 2);

            if (m_Header.channels > 1 && !std::isnan(sample.right))
            {
                channelvalue = sample.right * m_maxVal;
                m_WavFileStream.write((char*)&channelvalue, 2);
            }
        }
    }


    /////////////////////////////////////////////////
    /// \brief Read a sample from the audio stream.
    ///
    /// \return Sample
    ///
    /////////////////////////////////////////////////
    Sample WavFile::read() const
    {
        uint32_t samples[2] = {0,0};

        if (m_Header.channels == 1)
        {
            m_WavFileStream.read((char*)samples, m_Header.blockAlign);
            return Sample(convertFromUnsigned(samples[0] / m_maxVal));
        }
        else
        {
            m_WavFileStream.read((char*)&samples[0], m_Header.blockAlign/m_Header.channels);
            m_WavFileStream.read((char*)&samples[1], m_Header.blockAlign/m_Header.channels);
            return Sample(convertFromUnsigned(samples[0] / m_maxVal), convertFromUnsigned(samples[1] / m_maxVal));
        }

        return Sample(NAN);
    }


    /////////////////////////////////////////////////
    /// \brief Read a block of samples from the audio
    /// stream.
    ///
    /// \param len size_t
    /// \return std::vector<Sample>
    ///
    /////////////////////////////////////////////////
    std::vector<Sample> WavFile::readSome(size_t len) const
    {
        std::vector<Sample> vSamples;

        for (size_t i = 0; i < len; i++)
        {
            if (m_WavFileStream.eof() || !m_WavFileStream.good())
                break;

            vSamples.push_back(read());
        }

        return vSamples;
    }


    /////////////////////////////////////////////////
    /// \brief Write a block of samples to the audio
    /// stream.
    ///
    /// \param vSamples const std::vector<Sample>
    /// \return void
    ///
    /////////////////////////////////////////////////
    void WavFile::writeSome(const std::vector<Sample> vSamples)
    {
        for (const Sample& s : vSamples)
        {
            write(s);
        }
    }
}

