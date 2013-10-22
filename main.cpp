#include "mainwindow.h"
#include <QApplication>

#include "segd/csSegdReader.h"
//#include "io/csGeneralSeismicReader.h"
#include "segd/csSegdDefines.h"

int main(int argc, char *argv[])
{
    QApplication a(argc, argv);
    MainWindow w;
    w.show();
    
    cseis_segd::csSegdReader segdReader;
        // OTVORI FAJL:
        bool bRes = segdReader.open("/home/ernad/FFID-1481.segd");

        // Mislim da je ovo lose ime za funkciju. Ovje procita General Header 1, 2 i "n", sve Channel Sets i external heraders.
        // Cini mi se da na kraju procita i prvi Trace. Bez obzira, njega ne mozemo koristi jer to je jedan od ona 3 prva
        // koji nisu stvarni senzori.
        bRes = segdReader.readNewRecordHeaders();

        cseis_segd::commonRecordHeaderStruct comRecHdr;
        cseis_segd::commonTraceHeaderStruct comTrcHdr;

        // Ovo nema nikakve uloge. Samo sam nesto testirao.
        // int aaa = segdReader.numTraces();

        // !!! Ovjde ce biti memorisane informacije koje trebamo prikazati !!!!!!!!
        float fMeaurement[5000];
        //float fMeaurement[564][5000];

        // Ova funkcija nece naci nista zato sto cita Channel Set 1 (linija 0) sa 3 specijalne node.
        // "getNextTrace" poziva sebe rekurzivno dok ne nadje nodu cija tipa = 1 (seismic). Ako ne nadje a pregleda
        // sve Traces, returnira.
        segdReader.getNextTrace(fMeaurement, comTrcHdr);

        // Predji na novu linija sa senzorima:
        segdReader.readNextRecord(comRecHdr);
        // !!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!
        // Citamo trace za svaki senozor u ovom ChannelSet-u (564 vertikalnih linija po 4776 mjerenja po 4 bytes)
        // !!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!
        for (int i = 0; i < 564; i++) { // TODO: umjesto 564 bi trebalo procitati broj senzora u ChannelSet header-u
            segdReader.getNextTrace(fMeaurement, comTrcHdr);
        }

        // SADA MOZES KORISTITI fMeasurement da nacrtas graf.

        /*
        if (bRes)
            bRes = false;*/

    return a.exec();
}
