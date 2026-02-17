/*****************************************************************************
    NumeRe: Framework fuer Numerische Rechnungen
    Copyright (C) 2024  Erik Haenel et al.

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

#ifndef STRFUNCTIONIMPLEMENTATION_HPP
#define STRFUNCTIONIMPLEMENTATION_HPP

#include "../ParserLib/muParser.h"

mu::Array strfnc_to_tex(const mu::Array& a1, const mu::Array& a2); // OPT=1
mu::Array strfnc_to_uppercase(const mu::Array& a);
mu::Array strfnc_to_lowercase(const mu::Array& a);
mu::Array strfnc_utf8ToAnsi(const mu::Array& a);
mu::Array strfnc_ansiToUtf8(const mu::Array& a);
mu::Array strfnc_to_codepoints(const mu::Array& a);
mu::Array strfnc_from_codepoints(const mu::Array& a);
mu::Array strfnc_toHtml(const mu::Array& a);
mu::Array strfnc_getenvvar(const mu::Array& a);
mu::Array strfnc_getFileParts(const mu::Array& a);
mu::Array strfnc_getFileDiffs(const mu::Array& a1, const mu::Array& a2);
mu::Array strfnc_getfilelist(const mu::Array& a1, const mu::Array& a2); // OPT=1
mu::Array strfnc_getfolderlist(const mu::Array& a1, const mu::Array& a2); // OPT=1
mu::Array strfnc_strlen(const mu::Array& a);
mu::Array strfnc_firstch(const mu::Array& a);
mu::Array strfnc_lastch(const mu::Array& a);
mu::Array strfnc_getmatchingparens(const mu::Array& a);
mu::Array strfnc_ascii(const mu::Array& a);
mu::Array strfnc_isblank(const mu::Array& a);
mu::Array strfnc_isalnum(const mu::Array& a);
mu::Array strfnc_isalpha(const mu::Array& a);
mu::Array strfnc_iscntrl(const mu::Array& a);
mu::Array strfnc_isdigit(const mu::Array& a);
mu::Array strfnc_isgraph(const mu::Array& a);
mu::Array strfnc_islower(const mu::Array& a);
mu::Array strfnc_isprint(const mu::Array& a);
mu::Array strfnc_ispunct(const mu::Array& a);
mu::Array strfnc_isspace(const mu::Array& a);
mu::Array strfnc_isupper(const mu::Array& a);
mu::Array strfnc_isxdigit(const mu::Array& a);
mu::Array strfnc_isdir(const mu::Array& a);
mu::Array strfnc_isfile(const mu::Array& a);
mu::Array strfnc_to_char(const mu::MultiArgFuncParams& arrs);
mu::Array strfnc_findfile(const mu::Array& a1, const mu::Array& a2); // OPT=1
mu::Array strfnc_split(const mu::Array& a1, const mu::Array& a2, const mu::Array& a3); // OPT=1
mu::Array strfnc_to_time(const mu::Array& a1, const mu::Array& a2);
mu::Array strfnc_strfnd(const mu::Array& what, const mu::Array& where, const mu::Array& from); // OPT=1
mu::Array strfnc_strrfnd(const mu::Array& what, const mu::Array& where, const mu::Array& from); // OPT=1
mu::Array strfnc_strmatch(const mu::Array& chars, const mu::Array& where, const mu::Array& from); // OPT=1
mu::Array strfnc_strrmatch(const mu::Array& chars, const mu::Array& where, const mu::Array& from); // OPT=1
mu::Array strfnc_str_not_match(const mu::Array& chars, const mu::Array& where, const mu::Array& from); // OPT=1
mu::Array strfnc_str_not_rmatch(const mu::Array& chars, const mu::Array& where, const mu::Array& from); // OPT=1
mu::Array strfnc_strfndall(const mu::Array& what, const mu::Array& where, const mu::Array& from, const mu::Array& to); // OPT=2
mu::Array strfnc_strmatchall(const mu::Array& chars, const mu::Array& where, const mu::Array& from, const mu::Array& to); // OPT=2
mu::Array strfnc_findparam(const mu::Array& par, const mu::Array& line, const mu::Array& following); // OPT=1
mu::Array strfnc_substr(const mu::Array& sStr, const mu::Array& pos, const mu::Array& len); // OPT=1
mu::Array strfnc_repeat(const mu::Array& sStr, const mu::Array& rep);
mu::Array strfnc_timeformat(const mu::Array& fmt, const mu::Array& time);
mu::Array strfnc_weekday(const mu::Array& daynum, const mu::Array& opts); // OPT=1
mu::Array strfnc_char(const mu::Array& sStr, const mu::Array& pos);
mu::Array strfnc_getopt(const mu::Array& sStr, const mu::Array& pos);
mu::Array strfnc_replace(const mu::Array& where, const mu::Array& from, const mu::Array& len, const mu::Array& rep);
mu::Array strfnc_textparse(const mu::Array& sStr, const mu::Array& pattern, const mu::Array& p1, const mu::Array& p2); // OPT=2
mu::Array strfnc_locate(const mu::Array& arr, const mu::Array& tofind, const mu::Array& tol); // OPT=1
mu::Array strfnc_strunique(const mu::Array& arr, const mu::Array& opts); // OPT=1
mu::Array strfnc_strjoin(const mu::Array& arr, const mu::Array& sep, const mu::Array& opts); // OPT=2
mu::Array strfnc_getkeyval(const mu::Array& kvlist, const mu::Array& key, const mu::Array& defs, const mu::Array& opts); // OPT=2
mu::Array strfnc_findtoken(const mu::Array& sStr, const mu::Array& tok, const mu::Array& sep); // OPT=1
mu::Array strfnc_replaceall(const mu::Array& sStr, const mu::Array& fnd, const mu::Array& rep, const mu::Array& pos1, const mu::Array& pos2); // OPT=2
mu::Array strfnc_strip(const mu::Array& sStr, const mu::Array& frnt, const mu::Array& bck, const mu::Array& opts); // OPT=1
mu::Array strfnc_regex(const mu::Array& rgx, const mu::Array& sStr, const mu::Array& pos, const mu::Array& len); // OPT=2
mu::Array strfnc_basetodec(const mu::Array& base, const mu::Array& val);
mu::Array strfnc_dectobase(const mu::Array& base, const mu::Array& val);
mu::Array strfnc_justify(const mu::Array& arr, const mu::Array& align); // OPT=1
mu::Array strfnc_getlasterror();
mu::Array strfnc_geterrormessage(const mu::Array& errCode);
mu::Array strfnc_getversioninfo();
mu::Array strfnc_getuilang();
mu::Array strfnc_getuserinfo();
mu::Array strfnc_getuuid();
mu::Array strfnc_getodbcdrivers();
mu::Array strfnc_getfileinfo(const mu::Array& file);
mu::Array strfnc_sha256(const mu::Array& sStr, const mu::Array& opts); // OPT=1
mu::Array strfnc_encode_base_n(const mu::Array& sStr, const mu::Array& isFile, const mu::Array& n); // OPT=2
mu::Array strfnc_decode_base_n(const mu::Array& sStr, const mu::Array& n); // OPT=1
mu::Array strfnc_startswith(const mu::Array& sStr, const mu::Array& with);
mu::Array strfnc_endswith(const mu::Array& sStr, const mu::Array& with);
mu::Array strfnc_to_value(const mu::Array& sStr);
mu::Array strfnc_to_string(const mu::Array& vals);
mu::Array strfnc_getindices(const mu::Array& tab, const mu::Array& opts); // OPT=1
mu::Array strfnc_is_data(const mu::Array& sStr);
mu::Array strfnc_is_table(const mu::Array& sStr);
mu::Array strfnc_is_cluster(const mu::Array& sStr);
mu::Array strfnc_findcolumn(const mu::Array& tab, const mu::Array& col);
mu::Array strfnc_valtostr(const mu::Array& vals, const mu::Array& cfill, const mu::Array& len); // OPT=2
mu::Array strfnc_gettypeof(const mu::Array& vals);
mu::Array strfnc_readxml(const mu::Array& xmlFiles);
mu::Array strfnc_readjson(const mu::Array& jsonFiles);

#endif // STRFUNCTIONIMPLEMENTATION_HPP

