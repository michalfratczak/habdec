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

#include <string>
#include <thread>
#include <mutex>
#include <unordered_set>

#include <boost/asio/ip/tcp.hpp>
#include <boost/beast/core.hpp>
#include <boost/beast/websocket.hpp>
#include <boost/asio/signal_set.hpp>

#include "habdec_ws_protocol.h"


class websocket_session;

class WebsocketServer : public std::enable_shared_from_this<WebsocketServer>
{

public:
	WebsocketServer() = delete;
	WebsocketServer(const WebsocketServer&) = delete;
	WebsocketServer& operator=(const WebsocketServer&) = delete;

	WebsocketServer(std::string host, unsigned int port) : host_(host), port_(port) {}

	void operator()();

	// sessions
	void session_add  (websocket_session& session)		{ ws_sessions_.insert(&session); }
    void session_delete (websocket_session& session)	{ ws_sessions_.erase(&session);  }
    void sessions_send  (std::shared_ptr<HabdecMessage const> p_msg);

private:
	std::string 	host_{"0.0.0.0"};
	unsigned int	port_{5555};
	boost::asio::io_context ioc_{1};

	// sessions
	std::unordered_set<websocket_session*> ws_sessions_;
	std::mutex sessions_mtx_;
};
