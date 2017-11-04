#include "stdafx.h"
#include "Container.h"

Container::Container()
{

}

Container::Container(int frameRate) : m_frameRate(frameRate)
{
    ogg_int64_t        last_granulepos = 0;
    ogg_int64_t        enc_granulepos = 0;
    ogg_int64_t        original_samples = 0;
    ogg_int32_t        id = -1;

    int serialno = 997;
    if (ogg_stream_init(&m_streamState, serialno) == -1){
        fprintf(stderr, "Error: stream init failed\n");
        throw 1;
    }

    m_packet.b_o_s = 1;
    m_packet.e_o_s = 0;
    m_packet.granulepos = 0;
    m_packet.packetno = 0;


    m_file.open("audio.ogg", std::ios::out | std::ios::trunc | std::ios::binary);

}


Container::~Container()
{
    m_file.close();
}

int Container::storeHeader(unsigned char *data, int size)
{
    m_packet.bytes = size;
    m_packet.packet = data;
    m_packet.b_o_s = 1;
    m_packet.e_o_s = 0;
    m_packet.granulepos = 0;
    m_packet.packetno = 0;

    ogg_stream_packetin(&m_streamState, &m_packet);

    // Force Header data to be on separate page, apart from audio
    int written = 0;
    while ((written = ogg_stream_flush(&m_streamState, &m_page)))
    {
        if (!written)
        {
            break;
        }

        written += writePage();
    }

    return written;
}

int Container::storeData(unsigned char *data, int size)
{
    m_packet.packet = data;
    m_packet.bytes = size;
    m_packet.b_o_s = 0;
    m_packet.granulepos = m_packet.granulepos + size * 48000 / m_frameRate;
    ogg_stream_packetin(&m_streamState, &m_packet);

    while (ogg_stream_pageout(&m_streamState, &m_page))
    {
        //std::cout << "Container::storeData got page!\n";
        writePage();
    }
    
    return -1;
}


int Container::writePage()
{
    m_file.write((const char *)(m_page.header), m_page.header_len);
    m_file.write((const char *)(m_page.body), m_page.body_len);

    return m_page.header_len + m_page.body_len;
}
