#include "mainwindow.h"
#include "ui_mainwindow.h"
#include <QFileDialog>
#include <QMessageBox>
#include <QtAlgorithms>
#include "utility.h"
#include "model.h"
#include "segd/csSegdReader.h"
#include "segd/csSegdDefines.h"

// Simulation constants
QMap<QString, ChannelSet<float>* > channelSetMap;
QMap<QString, QPixmap*> channelSetPixmap;

const int channelSetNumber = 12;
const int nodeNumber = 564;
const int sampleNumber = 4776;

MainWindow::MainWindow(QWidget *parent) :
    QMainWindow(parent),
    ui(new Ui::MainWindow)
{
    ui->setupUi(this); //
    this->setFixedSize(this->size());

    generateRandomSeed();

    scene = new QGraphicsScene(this); 
    ui->graphicsView->setScene(scene);
}

MainWindow::~MainWindow()
{
    delete ui;
}

void generateDummyData(Ui::MainWindow *ui);
void readSegdFile(Ui::MainWindow *ui);
void MainWindow::on_actionOpen_triggered()
{
    //generateDummyData(ui);
    readSegdFile(ui);
    ui->channelSetComboBox->setFocus();
}

void addChannelSetPixmap(const QString &key, ChannelSet<float> *channelSet, Ui::MainWindow *ui)
{
    QPixmap *pixmap = new QPixmap(nodeNumber, sampleNumber);
    //pixmap->fill(Qt::white);
    QPainter painter(pixmap);

    long long total = nodeNumber * sampleNumber;
    long long current = 0;
    u_int8_t value;

    for (int i = 0; i < channelSet->getNodeSize(); i++)
    {
        for (int j = 0; j < channelSet->getSampleSize(); j++)
        {
            if ( (*channelSet)[i][j] < 0 )
                value = -(*channelSet)[i][j];
            else
                value = (*channelSet)[i][j];

            QPen pointPen(getColor(value));
            pointPen.setWidth(1);

            painter.setPen(pointPen);

            QPoint point(i, j);
            painter.drawPoint(point);
        }

        current += channelSet->getSampleSize();

        ui->progressBar->setValue(current * 100 / total);
    }

    channelSetPixmap.insert(key, pixmap);
}

template <typename T>
T** generateSamples(int nodeSize, int sampleSize)
{
    T** samples = 0;

    try
    {
        samples = new T*[nodeSize];

        samples[0] = new T[nodeSize * sampleSize];

        for (int i = 1; i < nodeSize; i++)
        {
            samples[i] = &samples[0][0] + i * sampleSize;
        }
    }
    catch (std::bad_alloc e)
    {
        if (samples != 0)
        {
            delete[] samples;
        }

        throw "Out of memory!";
    }

    return samples;
}


void readSegdFile(Ui::MainWindow *ui)
{
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
    float** samples = generateSamples<float>(nodeNumber, sampleNumber);
    //float fMeaurement[564][5000];

    // Ova funkcija nece naci nista zato sto cita Channel Set 1 (linija 0) sa 3 specijalne node.
    // "getNextTrace" poziva sebe rekurzivno dok ne nadje nodu cija tipa = 1 (seismic). Ako ne nadje a pregleda
    // sve Traces, returnira.
    segdReader.getNextTrace(samples[0], comTrcHdr);

    // Predji na novu linija sa senzorima:
    segdReader.readNextRecord(comRecHdr);
    // !!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!
    // Citamo trace za svaki senozor u ovom ChannelSet-u (564 vertikalnih linija po 4776 mjerenja po 4 bytes)
    // !!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!
    for (int i = 0; i < 564; i++) { // TODO: umjesto 564 bi trebalo procitati broj senzora u ChannelSet header-u
        segdReader.getNextTrace(samples[i], comTrcHdr);
    }

    // SADA MOZES KORISTITI fMeasurement da nacrtas graf.

    QString key = "Channel Set 01";

    channelSetMap.insert(key, new ChannelSet<float>(samples, nodeNumber, sampleNumber));

    QMessageBox::information(NULL, "File Read", "SEGD file read!");

    foreach (QString key, channelSetMap.keys())
    {
        ui->channelSetComboBox->addItem(key);
    }
}

/*
void generateDummyData(Ui::MainWindow *ui)
{
    //TODO: Dynamic allocation checking!
    long long total = channelSetNumber * nodeNumber * sampleNumber;
    long long current = 0;

    for (int i = 1; i <= channelSetNumber; i++)
    {
        int** samples = generateSamples(nodeNumber, sampleNumber);

        for (int j = 0; j < nodeNumber; j++)
        {
            for (int k = 0; k < sampleNumber; k++)
            {
                samples[j][k] = randomInteger(0, 255);
            }

            current += sampleNumber;
            ui->progressBar->setValue(current * 100 / total);
        }

        QString key;
        if (i < 10)
        {
            key = "Channel Set 0" + QString::number(i);
        }
        else
        {
            key = "Channel Set " + QString::number(i);
        }

        channelSetMap.insert(key, new ChannelSet<int>(samples, nodeNumber, sampleNumber));
    }

    QMessageBox::information(NULL, "File Read", "SEGD file read!");

    foreach (QString key, channelSetMap.keys())
    {
        ui->channelSetComboBox->addItem(key);
    }
}
*/

void MainWindow::on_channelSetComboBox_currentIndexChanged(const QString &key)
{
    ui->progressBar->setValue(0);

    // Create pixmap if not present in map
    if (channelSetPixmap.contains(key) == false)
    {
        addChannelSetPixmap(key, channelSetMap[key], ui);
    }

    // Removing previous items
    scene->clear();
scene->addPixmap(channelSetPixmap[key]->scaled(780, 490, Qt::IgnoreAspectRatio));
 //   scene->addPixmap(*channelSetPixmap[key]);

    ui->progressBar->setValue(100);
}
