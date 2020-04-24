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
#include <memory>
#include <cstddef>
#include <cstring>
#include <iostream>
#include <iomanip>
#include <stdio.h>
#include "../ssdv/ssdv.h"


static void print_hex(const std::array<uint8_t,256>& v)
{
    for(auto c : v)        std::cout<<"0x"<<std::hex<<(int)c<<" ";
    std::cout<<std::dec<<std::endl;
}


namespace habdec
{


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

    // make packet and decode header
    shared_ptr<packet_t> p_packet(new packet_t);
    memcpy( p_packet->data_.data(), buff_.data(), sizeof(p_packet->data_) );
    buff_.erase( buff_.begin(), buff_.begin() + SSDV_PKT_SIZE);
    packet_begin_ = -1;
    ssdv_dec_header( &(p_packet->header_), p_packet->data_.data() );

    //insert to image_ map
    pair<string,uint16_t> image_key(p_packet->header_.callsign_s, p_packet->header_.image_id);
    if( images_.find(image_key) == images_.end() )
        images_[image_key] = packet_set_t();
    auto& packet_list = images_[image_key];
    cout<<image_key.first<<" "<<image_key.second<<" "<<packet_list.size()<<endl;
    packet_list.insert(p_packet);

    // decode and save image
    {
        ssdv_t ssdv;
        ssdv_dec_init(&ssdv);
        uint8_t* jpeg = new uint8_t[1024*1024*3];
        ssdv_dec_set_buffer( &ssdv, jpeg, 1024*1024*3 );

        for(auto p_pkt : packet_list)
            ssdv_dec_feed( &ssdv, p_pkt->data_.data() );

        size_t jpeg_sz = 0;
        ssdv_dec_get_jpeg(&ssdv, &jpeg, &jpeg_sz);

        string fname =  "./ssdv_" + string(p_packet->header_.callsign_s)
                        + "_" + std::to_string(int(p_packet->header_.image_id)) + ".jpeg";
        FILE* fh = fopen( fname.c_str(), "wb" );
        fwrite(jpeg, 1, jpeg_sz, fh);
        fclose(fh);
        delete [] jpeg;

        if( (*packet_list.rbegin())->header_.eoi )
            images_.erase(image_key);
    }

    return true;
}

}