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


#include "ws_server.h"
#include "listener.h"
#include "websocket_session.h"


void WebsocketServer::operator()()
{
	using namespace std;
	using tcp = boost::asio::ip::tcp;

	auto const address = boost::asio::ip::make_address( host_ );
	auto const port = static_cast<unsigned short>( port_ );

	make_shared<listener>(
			ioc_,
			tcp::endpoint{address, port},
			*this
		)->run();

	ioc_.run();
}


void WebsocketServer::sessions_send(std::shared_ptr<HabdecMessage const> p_msg)
{
	{
		std::lock_guard<std::mutex> lock(sessions_mtx_);
		for(auto session : ws_sessions_)
			session->send(p_msg);
	}
}
