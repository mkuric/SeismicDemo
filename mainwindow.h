#ifndef MAINWINDOW_H
#define MAINWINDOW_H

#include <QMainWindow>
#include <QtGui>
#include <QtCore>
#include <QGraphicsScene>
#include "model.h"

class Projection;

namespace Ui {
class MainWindow;
}

class MainWindow : public QMainWindow
{
    Q_OBJECT
    
public:
    explicit MainWindow(QWidget *parent = 0);
    ~MainWindow();
    
private:
    Ui::MainWindow *ui;
    QGraphicsScene *scene;
    Projection* projection;

    void readSegdFile(Ui::MainWindow *ui);
    void repaintChannelSet(ChannelSet<float> *channelSet);
    void addChannelSetPixmap(const QString &key, ChannelSet<float> *channelSet, Ui::MainWindow *ui);

private slots:
    void on_actionOpen_triggered();
    void on_channelSetComboBox_currentIndexChanged(const QString &key);
};
#endif // MAINWINDOW_H
