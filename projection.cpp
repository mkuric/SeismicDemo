#include "projection.h"
#include "model.h"

Projection::Projection(float fStart_X, float fEnd_X, float fStart_Y, float fEnd_Y, int nPixSize_X, int nPixSize_Y)
{
    this->fStart_X = fStart_X;
    this->fEnd_X = fEnd_X;
    this->fStart_Y = fStart_Y;
    this->fEnd_Y = fEnd_Y;
    this->nPixSize_X = nPixSize_X;
    this->nPixSize_Y = nPixSize_Y;

    recalculateSize();
}

void Projection::recalculateSize() {
    fScale_X = (fEnd_X - fStart_X)/nPixSize_X;
    fScale_Y = (fEnd_Y - fStart_Y)/nPixSize_Y;
}

// Faster calculation but only one measurement used.
/*
float Projection::calculatePixValue(ChannelSet<float> *channelSet, int posX, int posY) {
    // TODO: is it better to declare fNode and fTime global (because of allocation time)? This fuction is used for each pixel
    float fNode = this->fStart_X + posX*fScale_X;
    float fTime  = this->fStart_Y + posY*fScale_Y;
    return (*channelSet)[(int)fNode][(int)fTime];
}*/


// TODO: for best scaling, rest of float should be used for calculation of the first and last element in for-loop
float Projection::calculatePixValue(ChannelSet<float> *channelSet, int posX, int posY) {
    // TODO: is it better to declare fNode and fTime global (because of allocation time)? This fuction is used for each pixel

    // Because of scaling, one pixel cover areal from "fNodeStart to" "fNodeEnd" (in X-axis)
    float fNodeStart = this->fStart_X + posX*fScale_X;
    float fNodeEnd = this->fStart_X + (posX + 1)*fScale_X;

    // Because of scaling, one pixel cover areal from "fTimeStart to" "fTimeEnd" (in Y-axis)
    float fTimeStart  = this->fStart_Y + posY*fScale_Y;
    float fTimeEnd  = this->fStart_Y + (posY + 1)*fScale_Y;

    int nNodeStart  = (int)(fNodeStart + 0.5f);  // 0.5f is round "float" to nearest "int"
    int nNodeEnd    = (int)(fNodeEnd   + 0.5f);  // 0.5f is round "float" to nearest "int"
    int nTimeStart  = (int)(fTimeStart + 0.5f);
    int nTimeEnd    = (int)(fTimeEnd   + 0.5f);

    float fSum = 0;

    for (int j = nTimeStart; j <= nTimeEnd; j++) {
        for (int i = nNodeStart; i <= nNodeEnd; i++) {
            fSum += (*channelSet)[i][j];
        }
    }

    return fSum / ((nNodeEnd - nNodeStart + 1)*(nTimeEnd - nTimeEnd + 1));
}

void Projection::resize_X_axis(float fStart_X, float fEnd_X) {
    this->fStart_X = fStart_X;
    this->fEnd_X = fEnd_X;

    recalculateSize();
}

void Projection::resize_Y_axis(float fStart_Y, float fEnd_Y) {
    this->fStart_Y = fStart_Y;
    this->fEnd_Y = fEnd_Y;

    recalculateSize();
}

void Projection::resize_XY_axis(float fStart_X, float fEnd_X, float fStart_Y, float fEnd_Y) {
    this->fStart_X = fStart_X;
    this->fEnd_X = fEnd_X;
    this->fStart_Y = fStart_Y;
    this->fEnd_Y = fEnd_Y;

    recalculateSize();
}

void Projection::resizePixelSize(int nPixSize_X, int nPixSize_Y) {
    this->nPixSize_X = nPixSize_X;
    this->nPixSize_Y = nPixSize_Y;

    recalculateSize();
}
