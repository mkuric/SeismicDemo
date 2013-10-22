#include "mainwindow.h"
#include "ui_mainwindow.h"
#include <QFileDialog>
#include <QMessageBox>
#include <QtAlgorithms>
#include "utility.h"
#include "model.h"

// Simulation constants
QMap<QString, ChannelSet<int>* > channelSetMap;
QMap<QString, QPixmap*> channelSetPixmap;

const int channelSetNumber = 12;
const int nodeNumber = 564;
const int sampleNumber = 4776;

MainWindow::MainWindow(QWidget *parent) :
    QMainWindow(parent),
    ui(new Ui::MainWindow)
{
    ui->setupUi(this);
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
void MainWindow::on_actionOpen_triggered()
{
    generateDummyData(ui);
    ui->channelSetComboBox->setFocus();
}

void addChannelSetPixmap(const QString &key, ChannelSet<int> *channelSet, Ui::MainWindow *ui)
{
    QPixmap *pixmap = new QPixmap(nodeNumber, sampleNumber);
    pixmap->fill(Qt::white);
    QPainter painter(pixmap);

    long long total = nodeNumber * sampleNumber;
    long long current = 0;

    for (int i = 0; i < channelSet->getNodeSize(); i++)
    {
        for (int j = 0; j < channelSet->getSampleSize(); j++)
        {

            u_int8_t value = (*channelSet)[i][j];

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

int** generateSamples(int nodeSize, int sampleSize)
{
    int** samples = 0;

    try
    {
        samples = new int*[nodeSize];

        samples[0] = new int[nodeSize * sampleSize];

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

    scene->addPixmap(*channelSetPixmap[key]);

    ui->progressBar->setValue(100);
}
