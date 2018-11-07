
#include "http_request.h"

#include <iostream>
#include <string>
#include <vector>
#include <map>
#include <sstream>
#include <chrono>
#include <thread>

#include <boost/beast/core.hpp>
#include <boost/beast/http.hpp>
#include <boost/beast/version.hpp>
#include <boost/asio/connect.hpp>
#include <boost/asio/ip/tcp.hpp>

namespace habdec
{

int HttpRequest(
        const std::string host,
        const std::string target,
        const int port,
        habdec::HTTP_VERB i_verb,
        const std::string i_content_type,
        const std::string i_body,
        std::string& o_body
    )
{
    using namespace std;
    using tcp = boost::asio::ip::tcp;
    namespace http = boost::beast::http;

    o_body = "";

    try
    {
        boost::asio::io_context ioc;
        tcp::resolver 	resolver{ioc};
        tcp::socket 	socket{ioc};
        auto const results = resolver.resolve(host, std::to_string(port));

        boost::asio::connect(socket, results.begin(), results.end());

        http::request<http::string_body> req; //{_verb, target, 10/*version*/};
        req.set(http::field::host, host);
        req.set(http::field::user_agent, "habdec");
        req.version(10); // 11 ?
        req.target(target);
        if(i_verb == HTTP_VERB::kGet)
            req.method( http::verb::get );
        else if(i_verb == HTTP_VERB::kPost)
            req.method( http::verb::post );
        if( i_content_type != "" )
            req.set(http::field::content_type, i_content_type);
        if( i_body != "" )
            req.body() = i_body;
        req.prepare_payload();

        http::write(socket, req);


        boost::beast::flat_buffer 			buffer;
        http::response<http::dynamic_body> 	res;
        try {
            http::read(socket, buffer, res);
        }
        catch(std::exception const& e) {
            o_body += e.what() + string("\n");
            o_body += string(res.reason());
            std::cerr << "Error: " << o_body << std::endl;
            return 0;
        }

        // std::cout << res <<std::endl;
		// for(auto const& field : res)			std::cout << field.name() << " = " << field.value() << "\n";

        // Gracefully close the socket
        boost::system::error_code ec;
        socket.shutdown(tcp::socket::shutdown_both, ec);

        // not_connected happens sometimes
        // so don't bother reporting it.
        //
        if(ec && ec != boost::system::errc::not_connected)
            throw boost::system::system_error{ec};
        // If we get here then the connection is closed gracefully

		o_body = buffers_to_string( res.body().data()) ;
		return res.result_int();
    }
    catch(std::exception const& e)
    {
		o_body = e.what();
        std::cerr << "Error: " << o_body << std::endl;
        return 0;
    }
    return 0;
}

} // namespace habdec
