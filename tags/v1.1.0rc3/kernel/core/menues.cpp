#include "menues.hpp"


/*
 * Menue-Funktionen
 * -> Nichts Spektakulaeres. Im Wesentlichen ein aehnliches Menue wie schon oben, nur dass die Menuepunkte etwas erlaeutert werden
 */
void menue_main(Settings& _option)
{
    // --> Mach' ein nettes Menue <--
    make_hline(-1);
    cerr << "|-> NUMERE: HAUPTMENUE (Praezision: " << _option.getPrecision() << ")" << endl;
    make_hline(-1);
    cerr << "|---<1> OPTIONEN" << endl;
    cerr << "|-------<11> Einstellungen" << endl;
    cerr << "|-------<12> Test-Tabelle (v " << PI_TT << ")" << endl;
    cerr << "|-------<13> Credits" << endl;
    cerr << "|---<2> DATENVERWALTUNG" << endl;
    cerr << "|-------<21> Daten aus Datei laden" << endl;
    cerr << "|-------<22> importierte Daten/Cache anzeigen" << endl;
    cerr << "|-------<23> Tabellenkoepfe bearbeiten" << endl;
    cerr << "|-------<24> Datenreihe hinzufuegen" << endl;
    cerr << "|-------<25> gespeicherte Daten verwerfen" << endl;
    cerr << "|-------<26> Cache leeren" << endl;
    cerr << "|---<3> DATENAUSWERTUNG" << endl;
    cerr << "|-------<31> Statistik (v " << PI_MED << ")" << endl;
    cerr << "|-------<32> Histogramm (v " << PI_HIST << ")" << endl;
    cerr << "|-------<33> Zufallszahlengenerator (v " << PI_RAND << ")" << endl;
    cerr << "|---<4> RECHNER-MODUS (v " << sParserVersion << ")" << endl;
    make_hline(-1);
    cerr << "|0> NUMERE BEENDEN" << endl;
    make_hline(-1);
    return;
}

void menue_settings(Settings& _option)
{
    make_hline(-1);
    cerr << "|1> NUMERE: OPTIONEN (Praezision: " << _option.getPrecision() << ")" << endl;
    make_hline(-1);
    cerr << "|---<11> EINSTELLUNGEN" << endl;
    cerr << "|        - Praezision einstellen" << endl;
    cerr << "|        - Cache einstellen" << endl;
    cerr << "|        - Standardspeicherplatz waehlen" << endl;
    cerr << "|        - Standard-Import-Ordner waehlen" << endl;
    cerr << "|---<12> TEST-TABELLE (v " << PI_TT << ")" << endl;
    cerr << "|        - Generiert eine einfache Tabelle mit" << endl;
    cerr << "|          frei waehlbaeren Zeilen- und Spalten-" << endl;
    cerr << "|          zahlen." << endl;
    cerr << "|        - Waehlen, um die Ausgabe zu pruefen." << endl;
    cerr << "|---<13> CREDITS" << endl;
    cerr << "|        - Rechtliche Informationen, Verantwort-" << endl;
    cerr << "|          liche, Garantie" << endl;
    make_hline(-1);
    cerr << "|0> NUMERE BEENDEN" << endl;
    make_hline(-1);
	return;
}

void menue_data(Settings& _option)
{
    make_hline(-1);
    cerr << "|2> NUMERE: DATENVERWALTUNG (Praezision: " << _option.getPrecision() << ")" << endl;
    make_hline(-1);
    cerr << "|---<21> DATEN AUS DATEI LADEN" << endl;
    cerr << "|        - Daten aus einem ASCII-File laden" << endl;
    cerr << "|        - Erlaubte Endungen: *.dat und *.txt" << endl;
    cerr << "|---<22> IMPORTIERTE DATEN/CACHE ANZEIGEN" << endl;
    cerr << "|        - Generiert eine Tabelle aus den" << endl;
    cerr << "|          importierten Werten oder dem Cache" << endl;
    cerr << "|        - Tabelle in ASCII-File exportieren" << endl;
    cerr << "|---<23> TABELLENKOEPFE BEARBEITEN" << endl;
    cerr << "|        - Kopfzeilen der gespeicherten Daten" << endl;
    cerr << "|          bearbeiten" << endl;
    cerr << "|---<24> DATENREIHE HINZUFUEGEN" << endl;
    cerr << "|        - Weitere Datenreihen zu einem bestehen-" << endl;
    cerr << "|          den Datensatz hinzufuegen" << endl;
    cerr << "|---<25> GESPEICHERTE DATEN VERWERFEN" << endl;
    cerr << "|        - Speicherplatz fuer neue Daten" << endl;
    cerr << "|          freigeben" << endl;
    cerr << "|---<26> CACHE LEEREN" << endl;
    cerr << "|        - Nicht mehr benoetigte Daten aus dem" << endl;
    cerr << "|          Speicher loeschen und den Cache" << endl;
    cerr << "|          zuruecksetzen." << endl;
    make_hline(-1);
    cerr << "|0> NUMERE BEENDEN" << endl;
    make_hline(-1);
	return;
}

void menue_statistics(Settings& _option)
{
    make_hline(-1);
    cerr << "|3> NUMERE: DATENAUSWERTUNG (Praezision: " << _option.getPrecision() << ")" << endl;
    make_hline(-1);
    cerr << "|---<31> STATISTIK (v " << PI_MED << ")" << endl;
    cerr << "|        - Berechnet die Mittelwerte, Standard-" << endl;
    cerr << "|          abweichungen, Unsicherheiten und den" << endl;
    cerr << "|          Prozentsatz der im Vertrauensintervall" << endl;
    cerr << "|          liegenden Punkte einer Tabelle." << endl;
    cerr << "|        - Benoetigt ein Datenfile." << endl;
    cerr << "|---<32> HISTOGRAMM (v " << PI_HIST << ")" << endl;
    cerr << "|        - Dieses Plugin generiert den Datensatz" << endl;
    cerr << "|          eines Histogramms aus einer gegeben" << endl;
    cerr << "|          Datenreihe." << endl;
    cerr << "|        - Benoetigt ein Datenfile." << endl;
    cerr << "|---<33> ZUFALLSZAHLENGENERATOR (v " << PI_RAND << ")" << endl;
    cerr << "|        - Generiert Zufallszahlen mit vorge-" << endl;
    cerr << "|          gebener Breite und Mittelwert." << endl;
    make_hline(-1);
    cerr << "|0> NUMERE BEENDEN" << endl;
    make_hline(-1);
	return;
}


