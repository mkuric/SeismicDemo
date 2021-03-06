#include "utility.h"
#include <QTime>

int randomInteger(int low, int high)
{
    return qrand() % ((high + 1) - low) + low;
}

void generateRandomSeed()
{
    QTime time = QTime::currentTime();
    qsrand((uint)time.msec());
}


QColor getColor(u_int8_t value)
{
    //////////////////////////////////////
    // Used as Gain = 4. TODO: ...
    int temp = value*4;
    if (temp > 255)
        temp = 255;

    value = temp;
    //////////////////////////////////////
    QColor orange(255, 128, 0);
    QColor indigo(75, 0, 130);
    QColor violet(148, 0, 211);

    QColor rainbowTable[] = {Qt::white, Qt::red, Qt::blue, indigo, violet, Qt::black};
    const int rainbowTableSize = 6;

    float step = 256.0f / rainbowTableSize;

    float alpha = value / step;
    int index = (int) alpha;
    alpha = alpha - index;

    int red = (1.0f - alpha) * rainbowTable[index].red() + alpha * rainbowTable[index + 1].red();
    int green = (1.0f - alpha) * rainbowTable[index].green() + alpha * rainbowTable[index + 1].green();
    int blue = (1.0f - alpha) * rainbowTable[index].blue() + alpha * rainbowTable[index + 1].blue();

    return QColor(red, green, blue);
}

unsigned int getUintColor(u_int8_t value)
{
    //////////////////////////////////////
    // Used as Gain = 4. TODO: ...
  /*  int temp = value*4;
    if (temp > 255)
        temp = 255;

    value = temp;*/
    //////////////////////////////////////
    QColor orange(255, 128, 0);
    QColor indigo(75, 0, 130);
    QColor violet(148, 0, 211);

    QColor rainbowTable[] = {Qt::white, Qt::red, Qt::blue, indigo, violet, Qt::black};
    const int rainbowTableSize = 6;

    float step = 256.0f / rainbowTableSize;

    float alpha = value / step;
    int index = (int) alpha;
    alpha = alpha - index;

    int red = (1.0f - alpha) * rainbowTable[index].red() + alpha * rainbowTable[index + 1].red();
    int green = (1.0f - alpha) * rainbowTable[index].green() + alpha * rainbowTable[index + 1].green();
    int blue = (1.0f - alpha) * rainbowTable[index].blue() + alpha * rainbowTable[index + 1].blue();

    return red << 16 | green << 8 | blue;
}
