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

#include <memory>
#include <string>

#include <boost/asio.hpp>

#include "ws_server.h"

namespace net = boost::asio;                    // namespace asio
using tcp = net::ip::tcp;                       // from <boost/asio/ip/tcp.hpp>
using error_code = boost::system::error_code;   // from <boost/system/error_code.hpp>


class listener : public std::enable_shared_from_this<listener>
{
    tcp::acceptor   acceptor_;
    tcp::socket     socket_;
    std::shared_ptr<WebsocketServer>   p_ws_server_;

    void fail(error_code ec, char const* what);
    void on_accept(error_code ec);

public:
    listener(net::io_context& ioc, tcp::endpoint endpoint, WebsocketServer& ws_server);
    void run();
};
