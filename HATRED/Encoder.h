#pragma once
#include <opus.h>
#include <memory>

class Encoder
{
public:
    Encoder(int bitrate, int channels);
    virtual ~Encoder();

    void encode(opus_int16 *input, int length);

protected:
    OpusEncoder *m_enc;
    int m_frameSize;
    int m_channels;
};

