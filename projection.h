#ifndef PROJECTION_H
#define PROJECTION_H
#include "model.h"

class Projection
{
private:
    float fStart_X;
    float fEnd_X;
    float fStart_Y;
    float fEnd_Y;
    int nPixSize_X;
    int nPixSize_Y;

    float fScale_X;
    float fScale_Y;

    void recalculateSize();

public:
    Projection(float fStart_X, float fEnd_X, float fStart_Y, float fEnd_Y, int nPixSize_X, int nPixSize_y); 
    float calculatePixValue(ChannelSet<float> *channelSet, int posX, int posY);

    void resize_X_axis(float fStart_X, float fEnd_X);
    void resize_Y_axis(float fStart_Y, float fEnd_Y);
    void resize_XY_axis(float fStart_X, float fEnd_X, float fStart_Y, float fEnd_Y);
    void resizePixelSize(int nPixSize_X, int nPixSize_y);
};

#endif // PROJECTION_H
