#include "stdafx.h"
#include "Encoder.h"

#include <stdio.h>
#include <string>
#include <stdlib.h>
#include <stdexcept>
#include <iostream>

#define test_failed() throw 1;

#define MAX_PACKET (2048)
#define SAMPLES (48000*30)
#define SSAMPLES (SAMPLES/3)
#define MAX_FRAME_SAMP (5760)
#define PI (3.141592653589793238462643f)
#define RAND_SAMPLE(a) (a[fast_rand() % sizeof(a)/sizeof(a[0])])
#define IMIN(a,b) ((a) < (b) ? (a) : (b))   /**< Minimum int value.   */

int get_frame_size_enum(int frame_size, int sampling_rate)
{
    int frame_size_enum;

    if (frame_size == sampling_rate / 400)
        frame_size_enum = OPUS_FRAMESIZE_2_5_MS;
    else if (frame_size == sampling_rate / 200)
        frame_size_enum = OPUS_FRAMESIZE_5_MS;
    else if (frame_size == sampling_rate / 100)
        frame_size_enum = OPUS_FRAMESIZE_10_MS;
    else if (frame_size == sampling_rate / 50)
        frame_size_enum = OPUS_FRAMESIZE_20_MS;
    else if (frame_size == sampling_rate / 25)
        frame_size_enum = OPUS_FRAMESIZE_40_MS;
    else if (frame_size == 3 * sampling_rate / 50)
        frame_size_enum = OPUS_FRAMESIZE_60_MS;
    else if (frame_size == 4 * sampling_rate / 50)
        frame_size_enum = OPUS_FRAMESIZE_80_MS;
    else if (frame_size == 5 * sampling_rate / 50)
        frame_size_enum = OPUS_FRAMESIZE_100_MS;
    else if (frame_size == 6 * sampling_rate / 50)
        frame_size_enum = OPUS_FRAMESIZE_120_MS;
    else
        test_failed();

    return frame_size_enum;
}

/*MWC RNG of George Marsaglia*/
static opus_uint32 Rz, Rw;
static OPUS_INLINE opus_uint32 fast_rand(void)
{
    Rz = 36969 * (Rz & 65535) + (Rz >> 16);
    Rw = 18000 * (Rw & 65535) + (Rw >> 16);
    return (Rz << 16) + Rw;
}

void generate_music(short *buf, opus_int32 len)
{
    opus_int32 a1, b1, a2, b2;
    opus_int32 c1, c2, d1, d2;
    opus_int32 i, j;
    a1 = b1 = a2 = b2 = 0;
    c1 = c2 = d1 = d2 = 0;
    j = 0;
    /*60ms silence*/
    for (i = 0; i<2880; i++)buf[i * 2] = buf[i * 2 + 1] = 0;
    for (i = 2880; i<len; i++)
    {
        opus_uint32 r;
        opus_int32 v1, v2;
        v1 = v2 = (((j*((j >> 12) ^ ((j >> 10 | j >> 12) & 26 & j >> 7))) & 128) + 128) << 15;
        r = fast_rand(); v1 += r & 65535; v1 -= r >> 16;
        r = fast_rand(); v2 += r & 65535; v2 -= r >> 16;
        b1 = v1 - a1 + ((b1 * 61 + 32) >> 6); a1 = v1;
        b2 = v2 - a2 + ((b2 * 61 + 32) >> 6); a2 = v2;
        c1 = (30 * (c1 + b1 + d1) + 32) >> 6; d1 = b1;
        c2 = (30 * (c2 + b2 + d2) + 32) >> 6; d2 = b2;
        v1 = (c1 + 128) >> 8;
        v2 = (c2 + 128) >> 8;
        buf[i * 2] = v1>32767 ? 32767 : (v1<-32768 ? -32768 : v1);
        buf[i * 2 + 1] = v2>32767 ? 32767 : (v2<-32768 ? -32768 : v2);
        if (i % 6 == 0)j++;
    }
}


void test_encode(OpusEncoder *enc, int channels, int frame_size, OpusDecoder *dec, const char* debug_info)
{
    int samp_count = 0;
    opus_int16 *inbuf;
    unsigned char packet[MAX_PACKET + 257];
    int len;
    opus_int16 *outbuf;
    int out_samples;

    /* Generate input data */
    inbuf = (opus_int16*)malloc(sizeof(*inbuf)*SSAMPLES);
    generate_music(inbuf, SSAMPLES / 2);

    /* Allocate memory for output data */
    outbuf = (opus_int16*)malloc(sizeof(*outbuf)*MAX_FRAME_SAMP * 3);

    /* Encode data, then decode for sanity check */
    do {
        len = opus_encode(enc, &inbuf[samp_count*channels], frame_size, packet, MAX_PACKET);
        if (len<0 || len>MAX_PACKET) {
            fprintf(stderr, "%s\n", debug_info);
            fprintf(stderr, "opus_encode() returned %d\n", len);
            test_failed();
        }

        out_samples = opus_decode(dec, packet, len, outbuf, MAX_FRAME_SAMP, 0);
        if (out_samples != frame_size) {
            fprintf(stderr, "%s\n", debug_info);
            fprintf(stderr, "opus_decode() returned %d\n", out_samples);
            test_failed();
        }

        samp_count += frame_size;
    } while (samp_count < ((SSAMPLES / 2) - MAX_FRAME_SAMP));

    /* Clean up */
    free(inbuf);
    free(outbuf);
}
void fuzz_encoder_settings(const int num_encoders, const int num_setting_changes)
{   
    //OpusDecoder *dec;
    //int i, j, err;

    ///* Parameters to fuzz. Some values are duplicated to increase their probability of being tested. */
    //int sampling_rates[5] = { 8000, 12000, 16000, 24000, 48000 };
    //int channels[2] = { 1, 2 };
    //int applications[3] = { OPUS_APPLICATION_AUDIO, OPUS_APPLICATION_VOIP, OPUS_APPLICATION_RESTRICTED_LOWDELAY };
    //int bitrates[11] = { 6000, 12000, 16000, 24000, 32000, 48000, 64000, 96000, 510000, OPUS_AUTO, OPUS_BITRATE_MAX };
    //int force_channels[4] = { OPUS_AUTO, OPUS_AUTO, 1, 2 };
    //int use_vbr[3] = { 0, 1, 1 };
    //int vbr_constraints[3] = { 0, 1, 1 };
    //int complexities[11] = { 0, 1, 2, 3, 4, 5, 6, 7, 8, 9, 10 };
    //int max_bandwidths[6] = { OPUS_BANDWIDTH_NARROWBAND, OPUS_BANDWIDTH_MEDIUMBAND,
    //    OPUS_BANDWIDTH_WIDEBAND, OPUS_BANDWIDTH_SUPERWIDEBAND,
    //    OPUS_BANDWIDTH_FULLBAND, OPUS_BANDWIDTH_FULLBAND };
    //int signals[4] = { OPUS_AUTO, OPUS_AUTO, OPUS_SIGNAL_VOICE, OPUS_SIGNAL_MUSIC };
    //int inband_fecs[3] = { 0, 0, 1 };
    //int packet_loss_perc[4] = { 0, 1, 2, 5 };
    //int lsb_depths[2] = { 8, 24 };
    //int prediction_disabled[3] = { 0, 0, 1 };
    //int use_dtx[2] = { 0, 1 };
    //int frame_sizes_ms_x2[9] = { 5, 10, 20, 40, 80, 120, 160, 200, 240 };  /* x2 to avoid 2.5 ms */
    //char debug_info[512];

    //for (i = 0; i<num_encoders; i++) {
    //    int sampling_rate = RAND_SAMPLE(sampling_rates);
    //    int num_channels = RAND_SAMPLE(channels);
    //    int application = RAND_SAMPLE(applications);

    //    dec = opus_decoder_create(sampling_rate, num_channels, &err);
    //    if (err != OPUS_OK || dec == NULL)test_failed();

    //    enc = opus_encoder_create(sampling_rate, num_channels, application, &err);
    //    if (err != OPUS_OK || enc == NULL)test_failed();

    //    for (j = 0; j<num_setting_changes; j++) {
    //        int bitrate = RAND_SAMPLE(bitrates);
    //        int force_channel = RAND_SAMPLE(force_channels);
    //        int vbr = RAND_SAMPLE(use_vbr);
    //        int vbr_constraint = RAND_SAMPLE(vbr_constraints);
    //        int complexity = RAND_SAMPLE(complexities);
    //        int max_bw = RAND_SAMPLE(max_bandwidths);
    //        int sig = RAND_SAMPLE(signals);
    //        int inband_fec = RAND_SAMPLE(inband_fecs);
    //        int pkt_loss = RAND_SAMPLE(packet_loss_perc);
    //        int lsb_depth = RAND_SAMPLE(lsb_depths);
    //        int pred_disabled = RAND_SAMPLE(prediction_disabled);
    //        int dtx = RAND_SAMPLE(use_dtx);
    //        int frame_size_ms_x2 = RAND_SAMPLE(frame_sizes_ms_x2);
    //        int frame_size = frame_size_ms_x2*sampling_rate / 2000;
    //        int frame_size_enum = get_frame_size_enum(frame_size, sampling_rate);
    //        force_channel = IMIN(force_channel, num_channels);

    //        sprintf_s(debug_info,
    //            "fuzz_encoder_settings: %d kHz, %d ch, application: %d, "
    //            "%d bps, force ch: %d, vbr: %d, vbr constraint: %d, complexity: %d, "
    //            "max bw: %d, signal: %d, inband fec: %d, pkt loss: %d%%, lsb depth: %d, "
    //            "pred disabled: %d, dtx: %d, (%d/2) ms\n",
    //            sampling_rate / 1000, num_channels, application, bitrate,
    //            force_channel, vbr, vbr_constraint, complexity, max_bw, sig, inband_fec,
    //            pkt_loss, lsb_depth, pred_disabled, dtx, frame_size_ms_x2);

    //        if (opus_encoder_ctl(enc, OPUS_SET_BITRATE(bitrate)) != OPUS_OK) test_failed();
    //        if (opus_encoder_ctl(enc, OPUS_SET_FORCE_CHANNELS(force_channel)) != OPUS_OK) test_failed();
    //        if (opus_encoder_ctl(enc, OPUS_SET_VBR(vbr)) != OPUS_OK) test_failed();
    //        if (opus_encoder_ctl(enc, OPUS_SET_VBR_CONSTRAINT(vbr_constraint)) != OPUS_OK) test_failed();
    //        if (opus_encoder_ctl(enc, OPUS_SET_COMPLEXITY(complexity)) != OPUS_OK) test_failed();
    //        if (opus_encoder_ctl(enc, OPUS_SET_MAX_BANDWIDTH(max_bw)) != OPUS_OK) test_failed();
    //        if (opus_encoder_ctl(enc, OPUS_SET_SIGNAL(sig)) != OPUS_OK) test_failed();
    //        if (opus_encoder_ctl(enc, OPUS_SET_INBAND_FEC(inband_fec)) != OPUS_OK) test_failed();
    //        if (opus_encoder_ctl(enc, OPUS_SET_PACKET_LOSS_PERC(pkt_loss)) != OPUS_OK) test_failed();
    //        if (opus_encoder_ctl(enc, OPUS_SET_LSB_DEPTH(lsb_depth)) != OPUS_OK) test_failed();
    //        if (opus_encoder_ctl(enc, OPUS_SET_PREDICTION_DISABLED(pred_disabled)) != OPUS_OK) test_failed();
    //        if (opus_encoder_ctl(enc, OPUS_SET_DTX(dtx)) != OPUS_OK) test_failed();
    //        if (opus_encoder_ctl(enc, OPUS_SET_EXPERT_FRAME_DURATION(frame_size_enum)) != OPUS_OK) test_failed();

    //        test_encode(enc, num_channels, frame_size, dec, debug_info);
    //    }

    //    opus_encoder_destroy(enc);
    //    opus_decoder_destroy(dec);
    //}
}

Encoder::Encoder(int samplingRate, int channels) : m_frameSize(960), m_channels(channels)
{
    int err;

    m_enc = opus_encoder_create(samplingRate, m_channels, OPUS_APPLICATION_AUDIO, &err);
                                

    if (err != OPUS_OK)
    {
        throw std::invalid_argument(std::string("Failed to create encoder"));
    }


    int status = opus_encoder_ctl(m_enc, OPUS_SET_BITRATE(OPUS_AUTO));

    if ( status != OPUS_OK)
    {
        throw std::invalid_argument("Failed to set bitrate status " + std::to_string(status));
    }

    if (opus_encoder_ctl(m_enc, OPUS_SET_FORCE_CHANNELS(OPUS_AUTO)) != OPUS_OK)
    {
        throw std::invalid_argument("Failed to create encoder for rate ");
    }

    if (opus_encoder_ctl(m_enc, OPUS_SET_VBR(1)) != OPUS_OK)
    {
        throw std::invalid_argument("Failed to create encoder for rate ");
    }

    if (opus_encoder_ctl(m_enc, OPUS_SET_VBR_CONSTRAINT(1)) != OPUS_OK)
    {
        throw std::invalid_argument("Failed to create encoder for rate ");
    }

    if (opus_encoder_ctl(m_enc, OPUS_SET_COMPLEXITY(5)) != OPUS_OK)
    {
        throw std::invalid_argument("Failed to create encoder for rate ");
    }

    if (opus_encoder_ctl(m_enc, OPUS_SET_MAX_BANDWIDTH(OPUS_BANDWIDTH_FULLBAND)) != OPUS_OK)
    {
        throw std::invalid_argument("Failed to create encoder for rate ");
    }

    if (opus_encoder_ctl(m_enc, OPUS_SET_SIGNAL(OPUS_SIGNAL_MUSIC)) != OPUS_OK)
    {
        throw std::invalid_argument("Failed to create encoder for rate ");
    }

    //if (opus_encoder_ctl(m_enc, OPUS_SET_INBAND_FEC(inband_fec)) != OPUS_OK)
    //{
    //    throw std::invalid_argument(std::string("Failed to create encoder for rate "));
    //}

    //if (opus_encoder_ctl(m_enc, OPUS_SET_PACKET_LOSS_PERC(pkt_loss)) != OPUS_OK)
    //{
    //    throw std::invalid_argument(std::string("Failed to create encoder for rate "));
    //}

    //if (opus_encoder_ctl(m_enc, OPUS_SET_LSB_DEPTH(lsb_depth)) != OPUS_OK)
    //{
    //    throw std::invalid_argument(std::string("Failed to create encoder for rate "));
    //}

    //if (opus_encoder_ctl(m_enc, OPUS_SET_PREDICTION_DISABLED(pred_disabled)) != OPUS_OK)
    //{
    //    throw std::invalid_argument(std::string("Failed to create encoder for rate "));
    //}

    if (opus_encoder_ctl(m_enc, OPUS_SET_DTX(1)) != OPUS_OK)
    {
        throw std::invalid_argument("Failed to create encoder for rate ");
    }

    //if (opus_encoder_ctl(m_enc, OPUS_SET_EXPERT_FRAME_DURATION(frame_size_enum)) != OPUS_OK)
    //{
    //    throw std::invalid_argument(std::string("Failed to create encoder for rate "));
    //}

}


Encoder::~Encoder()
{
    if (m_enc != nullptr)
    {
        opus_encoder_destroy(m_enc);

        m_enc = nullptr;
    }
}
//
//OPUS_EXPORT OPUS_WARN_UNUSED_RESULT opus_int32 opus_encode(
//    OpusEncoder *st,
//    const opus_int16 *pcm,
//    int frame_size,
//    unsigned char *data,
//    opus_int32 max_data_bytes
//    ) OPUS_ARG_NONNULL(1) OPUS_ARG_NONNULL(2) OPUS_ARG_NONNULL(4);


void Encoder::encode(opus_int16 *input, int length)
{
    unsigned char packet[MAX_PACKET];

    for (int i = 0; i < length / m_channels; i += m_frameSize)
    {
        int len = opus_encode(m_enc, input + i, m_frameSize, packet, MAX_PACKET);

        std::cout << "len " << len << "\n";

        if (len < 0 || len > MAX_PACKET)
        {
            throw std::invalid_argument("Failed to encode with status " + std::to_string(len));
        }
    }
    
}