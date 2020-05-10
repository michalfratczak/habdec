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

#include <cstdlib>
#include <memory>
#include <string>
#include <vector>
#include <mutex>

#include <boost/asio.hpp>
#include <boost/beast.hpp>

#include "ws_server.h"
#include "habdec_ws_protocol.h"


namespace net = boost::asio;                    // namespace asio
using tcp = net::ip::tcp;                       // from <boost/asio/ip/tcp.hpp>
using error_code = boost::system::error_code;   // from <boost/system/error_code.hpp>

class websocket_session : public std::enable_shared_from_this<websocket_session>
{
    boost::beast::flat_buffer buffer_;
    boost::beast::websocket::stream<tcp::socket> ws_;

    std::vector<std::shared_ptr<HabdecMessage const>> queue_;

    std::shared_ptr<WebsocketServer>   p_ws_server_;

    void fail(error_code ec, char const* what);
    void on_accept(error_code ec);
    void on_read(error_code ec, std::size_t bytes_transferred);
    void on_write(std::shared_ptr<std::string const> _unused, error_code ec, std::size_t);

    std::mutex	mtx_;

public:
    websocket_session(tcp::socket socket, std::shared_ptr<WebsocketServer> p_ws_server);
    ~websocket_session();

    template<class Body, class Allocator>
    void run(boost::beast::http::request<Body, boost::beast::http::basic_fields<Allocator>> req);

    void send(std::shared_ptr<HabdecMessage const> const& i_msg);
};

template<class Body, class Allocator>
void websocket_session::run(boost::beast::http::request<Body, boost::beast::http::basic_fields<Allocator>> req)
{
    ws_.async_accept(
        req,
        std::bind(
            &websocket_session::on_accept,
            shared_from_this(),
            std::placeholders::_1));
}
