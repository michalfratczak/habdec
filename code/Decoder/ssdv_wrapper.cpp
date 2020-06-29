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


namespace habdec
{


bool SSDV_wraper_t::push(const std::vector<char>& i_chars)
{
    using namespace std;

    // copy to buff_
    const size_t last_buff_end = buff_.size();
    buff_.resize( buff_.size() + i_chars.size() );
    memcpy( buff_.data() + last_buff_end, i_chars.data(), i_chars.size() );

    if( buff_.size() < SSDV_PKT_SIZE )
        return false;

    // scan buff_ for packet sync byte: 0x55
    if(packet_begin_ == -1)
    {
        while( ++packet_begin_ < buff_.size() && buff_[packet_begin_] != 0x55 ) /**/;

        if(packet_begin_ == buff_.size())
        {
            buff_.clear();
            packet_begin_ = -1;
            return false;
        }
    }

    if( (buff_.size()-packet_begin_) < SSDV_PKT_SIZE )
        return false;

    int errors = 0;
    const int is_packet = ssdv_dec_is_packet( buff_.data()+packet_begin_, &errors );
    if( is_packet != 0 ) // no packet starting at packet_begin_
    {
        //scan for another 0x55
        while( ++packet_begin_ < buff_.size() && buff_[packet_begin_] != 0x55 ) /**/;

        if(packet_begin_ == buff_.size())
        {
            buff_.clear();
            packet_begin_ = -1;
            return false;
        }
        else
        {
            buff_.erase( buff_.begin(), buff_.begin() + packet_begin_ );
            packet_begin_ = 0;
            return false;
        }
    }


    // make packet and decode header
    shared_ptr<packet_t> p_packet(new packet_t);
    memcpy( p_packet->data_.data(), buff_.data()+packet_begin_, sizeof(p_packet->data_) );
    buff_.erase( buff_.begin() + packet_begin_, buff_.begin() + packet_begin_ + SSDV_PKT_SIZE );
    packet_begin_ = -1;
    ssdv_dec_header( &(p_packet->header_), p_packet->data_.data() );

    // insert to packets_ map
    //  what happens when we already inserted packet with that ID ?
    //      either the packet is retransmitted for the same image
    //      or this indicates that a new image is being send with conflicting image num (over 255 cycle?)
    //       -> delete image and insert as new packet
    //  what happens when new packet has different resolution as previously inserted ?
    //      this indicates that a new image is being send with conflicting image num
    //       -> delete image and insert as new packet

    pair<string,uint16_t> image_key(p_packet->header_.callsign_s, p_packet->header_.image_id);
    auto p_packet_list = packets_.find(image_key);
    if( p_packet_list == packets_.end() ) // new image (callsign/id)
    {
        cout<<endl<<"   --   new image/packet set: "<<image_key.first<<" "<<image_key.second<<endl;
        packets_[image_key] = packet_set_t();
        packets_[image_key].insert(p_packet);
    }
    else  // append to existing image (callsign/id)
    {
        // check if this packet ID was already inserted
        bool packet_already_exists = false;
        for(auto _p_pkt : p_packet_list->second) {
            if(_p_pkt->header_.packet_id == p_packet->header_.packet_id) {
                packet_already_exists = true;
                break;
            }
        }

        // and check for resolution mismatch against previously inserted packet
        const auto last_height = p_packet_list->second.rbegin()->get()->header_.height;
        const auto last_width = p_packet_list->second.rbegin()->get()->header_.width;

        if(packet_already_exists) {
            cout<<endl<<"Packet ID "<<p_packet->header_.packet_id<<" for image "<<image_key.first<<"/"<<image_key.second
                <<" has already been received. Deleting and restarting image with new packet."<<endl;
            packets_[image_key] = packet_set_t();
            packets_[image_key].insert(p_packet);
        }
        else if(last_height != p_packet->header_.height || last_width != p_packet->header_.width ) {
            cout<<endl<<"Packet ID "<<p_packet->header_.packet_id<<" for image "<<image_key.first<<"/"<<image_key.second
                <<" has different resolution. Deleting and restarting image with new packet."<<endl;
            packets_[image_key] = packet_set_t();
            packets_[image_key].insert(p_packet);
        }
        else { // new packet has OK resolution and unique ID. Inserting.
            packets_[image_key].insert(p_packet);
        }
    }

    // decode and save image
    make_jpeg( packets_[image_key], image_key );
    save_jpeg(image_key);

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
