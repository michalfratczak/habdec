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

#pragma once

#include <array>
#include <vector>
#include <map>
#include <set>
#include <memory>
#include "../ssdv/ssdv.h"

namespace habdec
{

class SSDV_wraper_t
{
private:
    // incomming data buffer
    std::vector<uint8_t>    buff_;
    int packet_begin_ = -1;

    // ssdv packet with header
    struct packet_t {
        ssdv_packet_info_t          header_;
        std::array<uint8_t, 256>    data_;
    };

    using packet_t_ptr = std::shared_ptr<packet_t>;

    struct packet_t_ptr_less { // comparator
        bool operator()(const packet_t_ptr& lhs, const packet_t_ptr& rhs) {
            return lhs->header_.packet_id < rhs->header_.packet_id;
        }
    };

    using packet_set_t = std::set<packet_t_ptr, packet_t_ptr_less>;

    using image_key_t = std::pair<std::string,uint16_t>;

    using image_map_t = std::map<  // indexed by (callsign,imageID)
                            image_key_t,
                            packet_set_t >;

    // list of packets for each (callsign, imageId)
    std::map< image_key_t, packet_set_t >           packets_;

    // complete JPEG for each (callsign, imageId)
    std::map< image_key_t, std::vector<uint8_t> >   jpegs_;

    // output_image base filename
    std::string base_file_;

    void make_jpeg(const packet_set_t&, const image_key_t&);
    void save_jpeg(const image_key_t&);

public:
    bool push(const std::vector<char>& i_chars);
    std::string base_file() const { return base_file_; }
    void base_file(const std::string& i_fn) { base_file_ = i_fn; }
    image_key_t     last_img_k_ = {"",0};
    std::vector<uint8_t> get_jpeg(const image_key_t&);

};

}