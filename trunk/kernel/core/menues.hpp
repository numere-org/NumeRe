#include <iostream>
#include "settings.hpp"
#include "plotdata.hpp"

#ifndef MENUES_HPP
#define MENUES_HPP

extern const string PI_TT;
extern const string PI_HIST;
extern const string PI_MED;
extern const string PI_RAND;
extern const string sParserVersion;

/*
 * Menue-Prototypen
 * -> Generieren die "Untermenues"
 */
void menue_main(Settings&);
void menue_settings(Settings&);
void menue_data(Settings&);
void menue_statistics(Settings&);


void BI_hline(int nLength);
#endif
