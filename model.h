#ifndef MODEL_H
#define MODEL_H

template <typename T>
class ChannelSet
{
    T** _samples;
    int _nodeSize;
    int _sampleSize;

public:
    ChannelSet(T** samples, int nodeSize, int sampleSize) : _samples(samples), _nodeSize(nodeSize), _sampleSize(sampleSize)
    {

    }

    ~ChannelSet()
    {
        if (_samples != 0)
        {
            delete[] _samples[0];
            delete[] _samples;
        }
    }

    const T* operator[](int nodeId) const
    {
        return &_samples[nodeId][0];
    }

    int getNodeSize() const
    {
        return _nodeSize;
    }

    int getSampleSize() const
    {
        return _sampleSize;
    }
};
#endif // MODEL_H
