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

#include <boost/asio.hpp>
#include <boost/beast.hpp>

#include "ws_server.h"

namespace net = boost::asio;                    // namespace asio
using tcp = net::ip::tcp;                       // from <boost/asio/ip/tcp.hpp>
using error_code = boost::system::error_code;   // from <boost/system/error_code.hpp>



class http_session : public std::enable_shared_from_this<http_session>
{
    tcp::socket socket_;
    boost::beast::flat_buffer buffer_;
    boost::beast::http::request<boost::beast::http::string_body> req_;
    std::shared_ptr<WebsocketServer>   p_ws_server_;

    void fail(error_code ec, char const* what);
    void on_read(error_code ec, std::size_t);
    void on_write(error_code ec, std::size_t, bool close);

public:
    http_session(tcp::socket socket, std::shared_ptr<WebsocketServer> p_ws_server);
    void run();
};
