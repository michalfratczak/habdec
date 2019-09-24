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


#include "websocket_session.h"
#include "habdec_ws_protocol.h"
#include <iostream>

websocket_session::websocket_session(
    tcp::socket socket,
    std::shared_ptr<WebsocketServer> p_ws_server  )
        : ws_(std::move(socket))
{
    p_ws_server_ = p_ws_server;
}

websocket_session::~websocket_session()
{
    // Remove this session from the list of active sessions
    // state_->leave(*this);
    p_ws_server_->session_delete(*this);
}

void websocket_session::fail(error_code ec, char const* what)
{
    if( ec == net::error::operation_aborted ||
        ec == boost::beast::websocket::error::closed)
        return;
    std::cerr << what << ": " << ec.message() << "\n";
}

void websocket_session::on_accept(error_code ec)
{
    if(ec)
        return fail(ec, "accept");

    p_ws_server_->session_add(*this);

    // Read a message
    ws_.async_read(
        buffer_,
        [sp = shared_from_this()](
            error_code ec, std::size_t bytes)
        {
            sp->on_read(ec, bytes);
        });
}

void websocket_session::on_read(error_code ec, std::size_t)
{
    if(ec)
        return fail(ec, "read");

    std::string msg_str = boost::beast::buffers_to_string( buffer_.data() );
    buffer_.consume(buffer_.size());

    auto response_messages = HandleRequest(msg_str);
    for(const auto& p_msg : response_messages)
    {
        if(p_msg->to_all_clients_)
            p_ws_server_->sessions_send(p_msg);
        else
            this->send( p_msg );
    }

    ws_.async_read(
        buffer_,
        [sp = shared_from_this()](
            error_code ec, std::size_t bytes)
        {
            sp->on_read(ec, bytes);
        });
}

void websocket_session::send(std::shared_ptr<HabdecMessage const> const& i_msg)
{
    {
		std::lock_guard<std::mutex> _lock(mtx_);

        queue_.push_back(i_msg);

        if(queue_.size() > 1)
            return;

        ws_.binary(i_msg->is_binary_);

        ws_.async_write(
            net::buffer(queue_.front()->data_stream_.str()),
            [sp = shared_from_this()](error_code ec, std::size_t bytes)
            {
                sp->on_write(ec, bytes);
            });
    }
}


void websocket_session::on_write(error_code ec, std::size_t)
{
    if(ec)
        return fail(ec, "write");

    {
		std::lock_guard<std::mutex> _lock(mtx_);

        queue_.erase(queue_.begin());

        if(!queue_.empty())
        {
            ws_.binary(queue_.front()->is_binary_);
            ws_.async_write(
                net::buffer(queue_.front()->data_stream_.str()),
                [sp = shared_from_this()](error_code ec, std::size_t bytes)
                {
                    sp->on_write(ec, bytes);
                });
        }
    }
}
