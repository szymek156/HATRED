#pragma once
#include <opus.h>
#include <memory>
#include "Container.h"

class Encoder
{
public:
    Encoder(int bitrate, int channels);
    virtual ~Encoder();

    void encode(opus_int16 *input, int length);

protected:

    void writeHeader();

    // Values holds only for 48kHz, they has to be scaled otherwise
    enum class FrameMsToBytes
    {
        ms2_5 = 120,
        ms5 = 240,
        ms10 = 480,
        ms20 = 960,
        ms40 = 1920,
        ms60 = 2880
    };

    OpusEncoder *m_enc;
    int m_frameSize;
    int m_bitrate;
    int m_channels;

    std::unique_ptr<Container> m_container;
};

