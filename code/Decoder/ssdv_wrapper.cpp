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
#include <string>
#include <cstring>
#include <iostream>
#include <iomanip>
#include <future>
#include <ctime>
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
        if(i < buff_.size() ) {
            packet_begin_ = i;
        }
        else {
            buff_.clear();
            return false;
        }
    }

    if(packet_begin_>0)
    {
        buff_.erase( buff_.begin(), buff_.begin()+packet_begin_ );
        packet_begin_ = 0;
    }

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

    // insert to packets_ map
    pair<string,uint16_t> image_key(p_packet->header_.callsign_s, p_packet->header_.image_id);
    if( packets_.find(image_key) == packets_.end() )
        packets_[image_key] = packet_set_t();
    auto& packet_list = packets_[image_key];
    packet_list.insert(p_packet);

    // decode and save image
    make_jpeg(packet_list, image_key);
    save_jpeg(image_key);
    // if( (*packet_list.rbegin())->header_.eoi )
    //     packets_.erase(image_key);

    return true;
}


void SSDV_wraper_t::make_jpeg( const packet_set_t& packet_list, const image_key_t& image_key )
{
    using namespace std;

    auto& last_pkt = *packet_list.rbegin();
    size_t jpeg_sz = 3 * last_pkt->header_.width * last_pkt->header_.height;

    if( jpegs_.find(image_key) == jpegs_.end() )
        jpegs_[image_key] = vector<uint8_t>( jpeg_sz );

    auto& jpeg = jpegs_[image_key];
    uint8_t* p_data = &jpeg[0];

    ssdv_t ssdv;
    ssdv_dec_init(&ssdv);

    ssdv_dec_set_buffer( &ssdv, p_data, jpeg_sz );
    for(auto p_pkt : packet_list)
        ssdv_dec_feed( &ssdv, p_pkt->data_.data() );

    ssdv_dec_get_jpeg(&ssdv, &p_data, &jpeg_sz);

    last_img_k_ = image_key;
}

std::vector<uint8_t> SSDV_wraper_t::get_jpeg(const image_key_t& image_key)

{
    if( jpegs_.find(image_key) == jpegs_.end() ) {
        return std::vector<uint8_t>(0);
    }

    return jpegs_[image_key];
}



void SSDV_wraper_t::save_jpeg( const image_key_t& image_key )
{
    using namespace std;

    if( jpegs_.find(image_key) == jpegs_.end() )
        return;

    const auto jpeg = jpegs_[image_key];
    const auto base_file = base_file_;

    async(launch::async, [&jpeg, &base_file, image_key](){
        // timestamp
        auto t = std::time(nullptr);
        char timestamp[200];
        strftime(timestamp, 200, "%Y-%m-%d", std::localtime(&t) );
        // filename 0 padding
        string img_id_pad = std::to_string(image_key.second);
        img_id_pad = string(4 - img_id_pad.length(), '0') + img_id_pad;
        string fname =  base_file + string(timestamp)
                        + "_" + image_key.first
                        + "_" + img_id_pad + ".jpeg";

        FILE* fh = fopen( fname.c_str(), "wb" );
        fwrite(jpeg.data(), 1, jpeg.size(), fh);
        fclose(fh);
    });

}


} // namespace habdec
