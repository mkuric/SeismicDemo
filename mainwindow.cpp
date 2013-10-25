#include "mainwindow.h"
#include "ui_mainwindow.h"
#include <QFileDialog>
#include <QMessageBox>
#include <QtAlgorithms>
#include "utility.h"
#include "model.h"
#include "segd/csSegdReader.h"
#include "segd/csSegdDefines.h"

#include "segdutility.h"
#include "projection.h"

// Simulation constants
QMap<QString, ChannelSet<float>* > channelSetMap;
//QMap<QString, QPixmap*> channelSetPixmap;

// TODO: read this from headers
const int channelSetNumber = 12;
const int nodeNumber = 564;
const int sampleNumber = 4776;

int nWidth;
int nHeight;


MainWindow::MainWindow(QWidget *parent) :
    QMainWindow(parent),
    ui(new Ui::MainWindow)
{
    ui->setupUi(this); //
    //this->setFixedSize(this->size());

    generateRandomSeed();

    scene = new QGraphicsScene(this); 
    ui->graphicsView->setScene(scene);
    nWidth = ui->graphicsView->width();//780; //this->size().width();
    nHeight =  ui->graphicsView->height();// 490; //this->size().height();
    // TODO: change projection after reading of SEGD headers
    projection = new Projection(0.f, 563.f, 0.f, 4775.f, nWidth, nHeight);
}

MainWindow::~MainWindow()
{
    delete ui;
}

void generateDummyData(Ui::MainWindow *ui);

void MainWindow::on_actionOpen_triggered()
{
    //generateDummyData(ui);
    readSegdFile(ui);
    ui->channelSetComboBox->setFocus();
}

//////////////////////////////////////////////////////////////////////////////////////////
// Repeaint UI with measurements for this ChannelSet (line with channels)
//////////////////////////////////////////////////////////////////////////////////////////
void MainWindow::repaintChannelSet(ChannelSet<float> *channelSet) //, Ui::MainWindow *ui)
{
    QImage image(nWidth, nHeight, QImage::Format_RGB32);//Format_RGB888); // TODO: not working correct with RGB888
    //QPixmap *pixmap = new QPixmap(nodeNumber, sampleNumber);
    //pixmap->fill(Qt::white);
    //QPainter painter(pixmap);

    long long total = nHeight* nWidth; //nodeNumber * sampleNumber;
    long long current = 0;
    u_int8_t value;

    float fProjected;

    for (int j = 0; j < nHeight; j++)
    {
        for (int i = 0; i < nWidth; i++)
        {
            fProjected = projection->calculatePixValue(channelSet, i, j);

            if (fProjected < 0)
                value = - fProjected;
            else
                value = fProjected;

            unsigned int nVal = getUintColor(value);
            image.setPixel(i, j, nVal);
        }

        //current += channelSet->getSampleSize();
        //ui->progressBar->setValue(current * 100 / total);
    }

    scene->clear();

    scene->addPixmap(QPixmap::fromImage(image));
}

void MainWindow::readSegdFile(Ui::MainWindow *ui)
{
    cseis_segd::csSegdReader segdReader;
    // Open SEGD file. TODO: "Open File" button
    bool bRes = segdReader.open("/home/ernad/FFID-1481.segd");

    // TODO: Not good function name. Function is reading General Header 1, 2 i "n", all Channel Sets i external heraders.
    bRes = segdReader.readNewRecordHeaders();

    cseis_segd::commonRecordHeaderStruct comRecHdr;
    cseis_segd::commonTraceHeaderStruct comTrcHdr;


    //TODO: we are not using this measurements. Used only for reading first 3 trace (not a sensor measurement).
    float fMeasurement[sampleNumber];

    segdReader.getNextTrace(fMeasurement, comTrcHdr);

    segdReader.readNextRecord(comRecHdr);
    /////////////////////////////////////////////////////////////////////////////////////////////////////////////
    // Read trace for each node (Channel) in current ChannelSet (564 vertical lines with 4776 measurement(32 bits)
    for (int i = 1; i <= channelSetNumber; i++)
    {
        float** samples = generateSamples<float>(nodeNumber, sampleNumber);

        for (int j = 0; j < nodeNumber; j++)
        {
            // TODO:
            segdReader.getNextTrace(samples[j], comTrcHdr);
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

        channelSetMap.insert(key, new ChannelSet<float>(samples, nodeNumber, sampleNumber));
    }

    QMessageBox::information(NULL, "File Read", "SEGD file read!");

    foreach (QString key, channelSetMap.keys())
    {
        ui->channelSetComboBox->addItem(key);
    }
}

void MainWindow::on_channelSetComboBox_currentIndexChanged(const QString &key)
{
    ui->progressBar->setValue(0);

    repaintChannelSet(channelSetMap[key]);
}
