#ifndef SEGDUTILITY_H
#define SEGDUTILITY_H

#include <QtCore>

// Ernad: static template function must be declared in the header file ?
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

#endif // SEGDUTILITY_H
