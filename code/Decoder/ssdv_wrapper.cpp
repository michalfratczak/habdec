/*

	Copyright 2018 Michal Fratczak

	This file is part of habdec.

    habdec is free software: you can redistribute it and/or modify
    it under the terms of the GNU General Public License as published by
    the Free Software Foundation, either version 3 of the License, or
    (at your option) any later version.

    habdec is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
    GNU General Public License for more details.

    You should have received a copy of the GNU General Public License
    along with habdec.  If not, see <https://www.gnu.org/licenses/>.
*/

#include "ssdv_wrapper.h"
#include <cstddef>
#include <cstring>
#include <iostream>
#include <iomanip>
#include <stdio.h>
#include "../ssdv/ssdv.h"

void print_hex(const std::vector<uint8_t>& v)
{
    for(auto c : v)        std::cout<<"0x"<<std::hex<<(int)c<<" ";
    std::cout<<std::dec<<std::endl;
}

namespace habdec
{

SSDV_wraper_t::SSDV_wraper_t()
{
    jpeg = new uint8_t[1024 * 1024 * 3];
    ssdv_dec_init(&ssdv);
    ssdv_dec_set_buffer(&ssdv, jpeg, 1024*1024*3);
}


SSDV_wraper_t::~SSDV_wraper_t()
{
    delete [] jpeg;
}


bool SSDV_wraper_t::push(const std::vector<char>& i_chars)
{
    using namespace std;

    const size_t jpeg_data_sz_ = 1024 * 1024 * 3;

    // copy to buff_
    const size_t last_buff_end = buff_.size();
    buff_.resize( buff_.size() + i_chars.size() );
    memcpy( buff_.data() + last_buff_end, i_chars.data(), i_chars.size() );

    // scan newly appended input for packet sync byte: 0x55
    if(packet_begin_ == -1)
    {
        size_t i = last_buff_end;
        while( i < buff_.size() && buff_[i] != 0x55 )
            ++i;
        if(i < buff_.size() )
            packet_begin_ = i;
        else
            return false;
    }

    if(packet_begin_>0)
    {
        buff_.erase( buff_.begin(), buff_.begin()+packet_begin_ );
        packet_begin_ = 0;
    }

    // packet incomplete
    if( buff_.size() < SSDV_PKT_SIZE )
        return false;

    // print_hex(buff_);

    int errors = 0;
    const int is_packet = ssdv_dec_is_packet( buff_.data(), &errors );
    if( is_packet != 0 ) // no packet starting at packet_begin_
    {
        // lets not delete whole 256 bytes. packet could start somewhere there
        // buff_.erase( buff_.begin(), buff_.begin() + SSDV_PKT_SIZE );
        buff_.erase( buff_.begin(), buff_.begin() + 2 );
        packet_begin_ = -1;
        return false;
    }

    ssdv_packet_info_t packet_header;
    ssdv_dec_header( &packet_header, buff_.data() );

    cout<<"SSDV Packet "
        <<packet_header.packet_id<<" of image "<<(int)packet_header.image_id
        <<" MCU id "<<(int)packet_header.mcu_id
        <<" MCU offset "<<(int)packet_header.mcu_offset
        <<" EOI "<<(int)packet_header.eoi
        <<endl;

    if(0)
    {
        fprintf(stderr, "Decoded image packet. Callsign: %s, Image ID: %d, Packet ID: %d, Resolution: %dx%d, (%d errors corrected) "
                        "Type: %d, Quality: %d, EOI: %d, MCU Mode: %d, MCU Offset: %d, MCU ID: %d/%d\n",
            packet_header.callsign_s,
            packet_header.image_id,
            packet_header.packet_id,
            packet_header.width,
            packet_header.height,
            errors,
            packet_header.type,
            packet_header.quality,
            packet_header.eoi,
            packet_header.mcu_mode,
            packet_header.mcu_offset,
            packet_header.mcu_id,
            packet_header.mcu_count
        );
    }

    ssdv_dec_feed( &ssdv, buff_.data() );
    buff_.erase( buff_.begin(), buff_.begin() + SSDV_PKT_SIZE);
    packet_begin_ = -1;

    // save jpeg
    if(packet_header.eoi)
    {
        size_t jpeg_sz = 0;
        ssdv_dec_get_jpeg(&ssdv, &jpeg, &jpeg_sz);

        string fname = "./ssdv_" + string(packet_header.callsign_s) + "_" + std::to_string(int(packet_header.image_id)) + ".jpeg";
        FILE* fh = fopen( fname.c_str(), "wb" );
        fwrite(jpeg, 1, jpeg_sz, fh);
        fclose(fh);

        ssdv_dec_init(&ssdv);
        ssdv_dec_set_buffer(&ssdv, jpeg, 1024*1024*3);
    }

    return true;
}

}