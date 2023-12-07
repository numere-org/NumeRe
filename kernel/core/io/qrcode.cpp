/*****************************************************************************
    NumeRe: Framework fuer Numerische Rechnungen
    Copyright (C) 2023  Erik Haenel et al.

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

#include "qrcode.hpp"
#include "../../../externals/QR-Code-generator/cpp/qrcodegen.hpp"
#include "../../kernel.hpp"

#include <fstream>

using qrcodegen::QrCode;
using qrcodegen::QrSegment;

/////////////////////////////////////////////////
/// \brief Helper function to convert the created
/// QR code into an SVG file.
///
/// \param qr const QrCode&
/// \param border int
/// \param filename const std::string&
/// \return void
///
/////////////////////////////////////////////////
static void toSvg(const QrCode& qr, int border, const std::string& filename)
{
    if (border < 0)
        throw std::domain_error("Border must be non-negative");

    if (border > INT_MAX / 2 || border * 2 > INT_MAX - qr.getSize())
        throw std::overflow_error("Border too large");

    std::ofstream file(filename);

    if (!file.good())
        throw SyntaxError(SyntaxError::CANNOT_READ_FILE, filename, filename);

    file << "<?xml version=\"1.0\" encoding=\"UTF-8\"?>\n";
    file << "<!DOCTYPE svg PUBLIC \"-//W3C//DTD SVG 1.1//EN\" \"http://www.w3.org/Graphics/SVG/1.1/DTD/svg11.dtd\">\n";
    file << "<svg xmlns=\"http://www.w3.org/2000/svg\" version=\"1.1\" viewBox=\"0 0 ";
    file << (qr.getSize() + border * 2) << " " << (qr.getSize() + border * 2) << "\" stroke=\"none\">\n";
    file << "\t<rect width=\"100%\" height=\"100%\" fill=\"#FFFFFF\"/>\n";
    file << "\t<path d=\"";

    for (int y = 0; y < qr.getSize(); y++)
    {
        for (int x = 0; x < qr.getSize(); x++)
        {
            if (qr.getModule(x, y))
            {
                if (x != 0 || y != 0)
                    file << " ";

                file << "M" << (x + border) << "," << (y + border) << "h1v1h-1z";
            }
        }
    }

    file << "\" fill=\"#000000\"/>\n";
    file << "</svg>\n";
}


/////////////////////////////////////////////////
/// \brief Create a QR code from a string.
///
/// \param cmdParser CommandLineParser&
/// \return void
///
/////////////////////////////////////////////////
void createQrCode(CommandLineParser& cmdParser)
{
    const QrCode::Ecc level = QrCode::Ecc::LOW;
    const QrCode qr = QrCode::encodeText(cmdParser.parseExprAsString().c_str(), level);

    std::string sFileName = cmdParser.getFileParameterValueForSaving(".svg", "<savepath>", "<savepath>/qrcode.svg");

    toSvg(qr, 1, sFileName);

    if (NumeReKernel::getInstance()->getSettings().systemPrints())
        NumeReKernel::print(_lang.get("BUILTIN_NEW_FILECREATED", sFileName));
}




