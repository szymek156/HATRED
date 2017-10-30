#pragma once
#include <ogg/ogg.h>
#include <iostream>
#include <fstream>

class Container
{
public:
    Container();
    virtual ~Container();

    int storeHeader(unsigned char *data, int size);
    int storeData(unsigned char *data, int size);



protected:
    int writePage();
    std::ofstream m_file;

    ogg_stream_state   m_streamState;
    ogg_page           m_page;
    ogg_packet         m_packet;

};

