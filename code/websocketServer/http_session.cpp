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

#include "http_session.h"
#include "websocket_session.h"
#include <iostream>

http_session::http_session(
    tcp::socket socket,
    std::shared_ptr<WebsocketServer> p_ws_server)
        :   socket_(std::move(socket)),
            p_ws_server_(p_ws_server)
{
    std::cout<<"\n\nNew Client "<<socket_.remote_endpoint().address().to_string()<<std::endl<<std::endl;
}

void http_session::run()
{
    boost::beast::http::async_read(socket_, buffer_, req_,
        [self = shared_from_this()]
            (error_code ec, std::size_t bytes)
        {
            self->on_read(ec, bytes);
        });
}

void http_session::fail(error_code ec, char const* what)
{
    if(ec == net::error::operation_aborted)
        return;
    std::cerr << what << ": " << ec.message() << "\n";
}

void http_session::on_read(error_code ec, std::size_t)
{
    if(ec == boost::beast::http::error::end_of_stream)
    {
        socket_.shutdown(tcp::socket::shutdown_send, ec);
        return;
    }

    if(ec)
        return fail(ec, "read");

    if(boost::beast::websocket::is_upgrade(req_))
    {
        std::make_shared<websocket_session>(
            std::move(socket_), p_ws_server_
                )->run(std::move(req_));
        return;
    }

    // Send the response
    /*
    handle_request(state_->doc_root(), std::move(req_),
        [this](auto&& response)
        {
            // The lifetime of the message has to extend
            // for the duration of the async operation so
            // we use a shared_ptr to manage it.
            using response_type = typename std::decay<decltype(response)>::type;
            auto sp = std::make_shared<response_type>(std::forward<decltype(response)>(response));

            // Write the response
            auto self = shared_from_this();
            http::async_write(this->socket_, *sp,
				[self, sp](
					error_code ec, std::size_t bytes)
				{
					self->on_write(ec, bytes, sp->need_eof());
				});
        });
    */
}

void http_session::on_write(error_code ec, std::size_t, bool close)
{
    if(ec)
        return fail(ec, "write");

    if(close)
    {
        socket_.shutdown(tcp::socket::shutdown_send, ec);
        return;
    }

    // Clear contents of the request message,
    // otherwise the read behavior is undefined.
    req_ = {};

    boost::beast::http::async_read(socket_, buffer_, req_,
        [self = shared_from_this()]
            (error_code ec, std::size_t bytes)
        {
            self->on_read(ec, bytes);
        });
}
