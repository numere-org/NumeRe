#include "syntax.hpp"


NumeReSyntax::NumeReSyntax()
{
    vCommands = splitString("about append audio break clear compose cont cont3d continue contour contour3d copy credits datagrid define del delete dens dens3d density density3d diff draw draw3d edit eval explicit export extrema fft find fit fitw get global grad gradient grad3d gradient3d graph graph3d help hist hist2d hline ifndef ifndefined info integrate integrate2 integrate2d list load man matop mesh mesh3d meshgrid meshgrid3d move mtrxop new odesolve plot plot3d print progress pulse quit random read redef redefine regularize reload remove rename repl replaceline resample retoque save script search set show showf smooth sort stats start stfa subplot surf surf3d surface surface3d swap taylor undef undefine uninstall vect vect3d vector vector3d view workpath write zeroes else elseif endfor endif endwhile endcompose explicit for if inline main namespace private readline return throw while procedure endprocedure str var");
    vFunctions = splitString("abs acos acosh Ai arc arcv arccos arcsin arctan arcosh arsinh artanh asin asinh ascii atan atanh avg bessel betheweizsaecker Bi binom char circle cmp cnt cone conev cos cosh cot cross cuboid curve date dblfacul degree det diag diagonalize drop eigenvals eigenvects ellipse ellipsev ellipticD ellipticE ellipticF ellipticPi erf erfc exp face facev faculty findfile findparam floor gamma gauss gcd getfilelist getfolderlist getindices getmatchingparens getopt heaviside hermite identity imY invert is_data is_nan is_string laguerre laguerre_a lcm legendre legendre_a ln log log10 log2 matfc matfcf matfl matflf max med min neumann norm num one pct phi point polygon polygonv prd radian rand range rect repeat replace replaceall rint roof round sbessel sign sin sinc sinh sneumann solve split sphere sqrt std strfnd string_cast strlen strrfnd student_t substr sum tan tanh text theta time to_char to_cmd to_lowercase to_string to_uppercase to_value trace tracev transpose triangle trianglev valtostr version Y zero");
    vOptions = splitString("addxaxis addyaxis adventor all allmedium alpha alphamask animate app append area aspect asstr asval autosave axis axisbind background bar bars bcancel bgcolorscheme binlabel binomial bins bonum bottomleft bottomright box boxplot buffersize cancel cartesian chimap chorus clog cloudplot cmd coarse coast cold colorbar colormask colorrange colorscheme colortheme cols comment comments compact complete complex connect const copper coords countlabel cscale cticklabels cticks cursor cut defcontrol desc dir distrib draftmode editor eps errorbars every exprvar extendedfileinfo faststart fcont file files fine first flength flow font free freedman freedoms func fx0 gamma gauss greeting grey grid gridstyle hbars head heros heroscn hints hires hot ignore interpolate inverse iter keepdim last lborder lcont legend legendstyle light lines linesizes linestyles lnumctrl loademptycols loadpath logic logscale lyapunov map marks mask max maxline mean medium method min minline mode moy msg multiplot noalpha noalphamask noanimate noarea noaxis nobackground nobars nobox noboxplot noclog nocloudplot nocolorbar nocolormask noconnect nocut noerrorbars nofcont noflength noflow nogrid nohbars nohires nointerpolate nolcont nolight nologscale nomarks noopen noorthoproject nopcont nopipe nopoints noquotes noregion normal noschematic nosteps nosilent noxlog noyerrorbars noylog nozlog nq oeps ogif onlycolors onlystyles open opng order origin orthoproject osvg otex override pagella params pattern pcont peek perspective pipe plasma plotcolors plotfont plotparams plotpath plugin plugins points pointstyles poisson polar precision prob proc procpath rainbow rborder real recursive region reset restrict rk2 rk4 rk8pd rkck rkf45 rotate samples savepath saverr scale schematic schola scott scriptpath settings shape silent simpson single slices sliding spherical std steps student styles sv target termes textsize title tocache tol topleft topright transpose trapezoidal trunc type ubound uniform unique units usecustomlang useescinscripts var viridis viewer vline width windowsize with xerrorbars xlabel xlog xscale xticklabels xticks xvals xy xz yerrorbars ylabel ylog yscale yticklabels yticks zlabel zlog zscale zticklabels zticks");
    vConstants = splitString("_pi _2pi _g _R _alpha_fs _c _coul_norm _elek_feldkonst _elem_ladung _g _h _hbar _k_boltz _m_amu _m_elektron _m_erde _m_muon _m_neutron _m_proton _m_sonne _m_tau _magn_feldkonst _mu_bohr _mu_kern _n_avogadro _pi _r_bohr _r_erde _r_sonne _theta_weinberg");
    vSpecialValues = splitString("... ans cache data false inf nan string true void");
    vOperators = splitString("<loadpath> <savepath> <scriptpath> <procpath> <plotpath> <this> <wp> 'Torr 'eV 'fm 'A 'b 'AU 'ly 'pc 'mile 'yd 'ft 'in 'cal 'TC 'TF 'Ci 'G 'kmh 'kn 'l 'mph 'psi 'Ps 'mol 'Gs 'M 'k 'm 'mu 'n");
    sSingleOperators = "+-*/?=()[]{},;#^!&|<>%:";
}

string NumeReSyntax::constructString(const vector<string>& vVector) const
{
    string sReturn = "";

    for (size_t i = 0; i < vVector.size(); i++)
    {
        sReturn += vVector[i] + " ";
    }
    return sReturn;
}

vector<string> NumeReSyntax::splitString(string sString)
{
    vector<string> vReturn;
    while (sString.length())
    {
        while (sString.front() == ' ')
            sString.erase(0,1);
        if (sString.find(' ') != string::npos)
        {
            vReturn.push_back(sString.substr(0,sString.find(' ')));
            sString.erase(0,sString.find(' ')+1);

            if (!vReturn.back().length())
                vReturn.pop_back();
        }
        else
        {
            vReturn.push_back(sString);
            break;
        }
    }
    return vReturn;
}

bool NumeReSyntax::matchItem(const vector<string>& vVector, const string& sString)
{
    for (size_t i = 0; i < vVector.size(); i++)
    {
        if (vVector[i] == sString)
            return true;
    }
    return false;
}

string NumeReSyntax::highlightLine(const string& sCommandLine)
{
    string colors;
    colors.assign(sCommandLine.length(),'0'+SYNTAX_STD);
    char c;

    if (sCommandLine.substr(0,3) != "|<-"
        && sCommandLine.substr(0,5) != "|-\?\?>"
        && (sCommandLine.substr(0,2) != "||" || sCommandLine.substr(0,4) == "||->")
        && sCommandLine.substr(0,4) != "|FOR"
        && sCommandLine.substr(0,5) != "|ELSE"
        && sCommandLine.substr(0,5) != "|ELIF"
        && sCommandLine.substr(0,5) != "|PROC"
        && sCommandLine.substr(0,5) != "|COMP"
        && sCommandLine.substr(0,3) != "|IF"
        && sCommandLine.substr(0,4) != "|WHL")
        return colors;

    if (sCommandLine.find('"') != string::npos)
    {
        char c_string = '0'+SYNTAX_STRING;
        char c_normal = '0'+SYNTAX_STD;
        c = c_string;
        for (size_t k = sCommandLine.find('"'); k < sCommandLine.length(); k++)
        {
            if (c == c_normal && sCommandLine[k] == '"')
            {
                c = c_string;
                colors[k] = c;
                continue;
            }

            colors[k] = c;

            if (c == c_string && sCommandLine[k] == '"' && k > sCommandLine.find('"'))
            {
                c = c_normal;
            }
        }
    }
    if (sCommandLine.find('$') != string::npos)
    {
        int c_proc = '0'+SYNTAX_PROCEDURE;
        int c_normal = '0'+SYNTAX_STD;
        c = c_proc;
        for (size_t k = sCommandLine.find('$'); k < sCommandLine.length(); k++)
        {
            if (sCommandLine[k] == '(' || sCommandLine[k] == ' ')
            {
                c = c_normal;
            }
            else if (sCommandLine[k] == '$')
            {
                c = c_proc;
            }
            if (colors[k] == '0'+SYNTAX_STD)
                colors[k] = c;
            else
                c = c_normal;
        }
    }

    for (unsigned int i = 0; i < sCommandLine.length(); i++)
    {
        if (!i && sCommandLine.substr(0,3) == "|<-")
            i += 3;
        else if (!i && (sCommandLine.substr(0,5) == "|-\?\?>" || sCommandLine.substr(0,4) == "|FOR" || sCommandLine.substr(0,5) == "|ELSE" || sCommandLine.substr(0,5) == "|ELIF" || sCommandLine.substr(0,4) == "|WHL" || sCommandLine.substr(0,3) == "|IF" || sCommandLine.substr(0,5) == "|PROC" || sCommandLine.substr(0,5) == "|COMP"))
            i += sCommandLine.find('>')+1;
        else if (!i && sCommandLine.substr(0,4) == "||<-")
            i += 4;
        else if (!i && sCommandLine.substr(0,3) == "|| ")
            i += 3;
        if (sCommandLine[i] == ' ')
            continue;
        if (sCommandLine[i] >= '0' && sCommandLine[i] <= '9')
        {
            unsigned int nLen = 0;
            while (i+nLen < sCommandLine.length()
                && colors[i+nLen] == '0'+SYNTAX_STD
                && ((sCommandLine[i+nLen] >= '0' && sCommandLine[i+nLen] <= '9')
                    || sCommandLine[i+nLen] == '.'
                    || sCommandLine[i+nLen] == 'e'
                    || sCommandLine[i+nLen] == 'E'
                    || ((sCommandLine[i+nLen] == '-' || sCommandLine[i+nLen] == '+')
                        && (sCommandLine[i+nLen-1] == 'e' || sCommandLine[i+nLen-1] == 'E'))
                    )
                )
                nLen++;
            colors.replace(i, nLen, nLen, '0'+SYNTAX_NUMBER);
            i += nLen;
        }
        if (colors[i] == '0'+SYNTAX_STD)
        {
            unsigned int nLen = 0;
            while (i+nLen < sCommandLine.length()
                && colors[i+nLen] == '0'+SYNTAX_STD
                && sCommandLine[i+nLen] != ' '
                && sCommandLine[i+nLen] != '\''
                && sSingleOperators.find(sCommandLine[i+nLen]) == string::npos)
                nLen++;
            if (sSingleOperators.find(sCommandLine[i+nLen]) != string::npos)
            {
                colors[i+nLen] = '0'+SYNTAX_OPERATOR;
            }
            if (matchItem(vCommands, sCommandLine.substr(i,nLen)))
            {
                colors.replace(i, nLen, nLen, '0'+SYNTAX_COMMAND);
            }
            else if (i+nLen < sCommandLine.length()
                && sCommandLine[i+nLen] == '('
                && matchItem(vFunctions, sCommandLine.substr(i,nLen)))
            {
                colors.replace(i, nLen, nLen, '0'+SYNTAX_FUNCTION);
            }
            else if (matchItem(vOptions, sCommandLine.substr(i,nLen)))
            {
                colors.replace(i, nLen, nLen, '0'+SYNTAX_OPTION);
            }
            else if (matchItem(vConstants, sCommandLine.substr(i,nLen)))
            {
                colors.replace(i, nLen, nLen, '0'+SYNTAX_CONSTANT);
            }
            else if (matchItem(vSpecialValues, sCommandLine.substr(i,nLen)))
            {
                colors.replace(i, nLen, nLen, '0'+SYNTAX_SPECIALVAL);
            }
            i += nLen;
        }
    }

    return colors;
}

