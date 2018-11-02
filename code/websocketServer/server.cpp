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

#include <cstdlib>

#include <memory>
#include <string>
#include <iostream>
#include <thread>
#include <functional>
#include <regex>

#include <boost/beast/core.hpp>
#include <boost/beast/websocket.hpp>
#include <boost/asio/ip/tcp.hpp>
using boost::asio::ip::tcp;
namespace beast = 		boost::beast;
namespace websocket = 	boost::beast::websocket;


#include "common/console_colors.h"
#include "CompressedVector.h"
#include "GLOBALS.h"
#include "NetTransport.h"

extern bool G_DO_EXIT;


bool HandleCommand(const std::string i_command, websocket::stream<tcp::socket>& ws)
{
	if(!GLOBALS::get().p_iq_source_)
		return false;

	using namespace std;
	smatch match;

	// GET
	if(i_command == "get:frequency")
	{
		double frequency = 0;
		GLOBALS::get().p_iq_source_->getOption("frequency_double", &frequency);
		frequency /= 1e6;
		string o_command = "cmd::set:frequency=" + to_string(frequency);
		ws.write( boost::asio::buffer(o_command.c_str(), o_command.size()) );
	}
	else if(i_command == "get:gain")
	{
		double gain = 0;
		GLOBALS::get().p_iq_source_->getOption("gain_double", &gain);
		string o_command = "cmd::set:gain=" + to_string(gain);
		ws.write( boost::asio::buffer(o_command.c_str(), o_command.size()) );
	}
	else if(i_command == "get:baud")
	{
		size_t baud = GLOBALS::get().decoder_.baud();
		string o_command = "cmd::set:baud=" + to_string(baud);
		ws.write( boost::asio::buffer(o_command.c_str(), o_command.size()) );
	}
	else if(i_command == "get:rtty_bits")
	{
		size_t rtty_bits = GLOBALS::get().decoder_.rtty_bits();
		string o_command = "cmd::set:rtty_bits=" + to_string(rtty_bits);
		ws.write( boost::asio::buffer(o_command.c_str(), o_command.size()) );
	}
	else if(i_command == "get:rtty_stops")
	{
		float rtty_stops = GLOBALS::get().decoder_.rtty_stops();
		string o_command = "cmd::set:rtty_stops=" + to_string(rtty_stops);
		ws.write( boost::asio::buffer(o_command.c_str(), o_command.size()) );
	}
	else if(i_command == "get:lowpass_bw")
	{
		string o_command = "cmd::set:lowpass_bw=" + to_string(GLOBALS::get().decoder_.lowpass_bw());
		ws.write( boost::asio::buffer(o_command.c_str(), o_command.size()) );
	}
	else if(i_command == "get:lowpass_trans")
	{
		string o_command = "cmd::set:lowpass_trans=" + to_string(GLOBALS::get().decoder_.lowpass_trans());
		ws.write( boost::asio::buffer(o_command.c_str(), o_command.size()) );
	}
	else if(i_command == "get:biastee")
	{
		double biastee = 0;
		GLOBALS::get().p_iq_source_->getOption("biastee_double", &biastee);
		string o_command = "cmd::set:biastee=" + to_string(biastee);
		ws.write( boost::asio::buffer(o_command.c_str(), o_command.size()) );
	}
	else if(i_command == "get:afc")
	{
		string o_command = "cmd::set:afc=" + to_string(GLOBALS::get().afc_);
		ws.write( boost::asio::buffer(o_command.c_str(), o_command.size()) );
	}
	else if(i_command == "get:decimation")
	{
		int decim_factor_log = std::log2( GLOBALS::get().decoder_.getDecimationFactor() );
		string o_command = "cmd::set:decimation=" + to_string(decim_factor_log);
		ws.write( boost::asio::buffer(o_command.c_str(), o_command.size()) );
	}
	else if(i_command == "get:sampling_rate")
	{
		double sr = 0;
		GLOBALS::get().p_iq_source_->getOption("sampling_rate_double", &sr);
		string o_command = "cmd::info:sampling_rate=" + to_string(sr);
		ws.write( boost::asio::buffer(o_command.c_str(), o_command.size()) );
	}
	// SET
	else if( regex_match(i_command, match, regex(R"_(set\:frequency=([+-]?([0-9]*[.])?[0-9]+))_")) && match.size() > 1 )
	{
		double frequency = stod(match[1]);
		frequency *= 1e6;
		GLOBALS::get().p_iq_source_->setOption("frequency_double", &frequency);
		GLOBALS::get().frequency_ = frequency;

		string o_command = "cmd::set:frequency=" + to_string(frequency/1e6);
		ws.write( boost::asio::buffer(o_command.c_str(), o_command.size()) );
	}
	else if( regex_match(i_command, match, regex(R"_(set\:decimation=(\d+))_")) && match.size() > 1 )
	{
		int decim_factor_log = stoi(match[1]);
		GLOBALS::get().decoder_.setupDecimationStagesFactor( pow(2,decim_factor_log) );
		string o_command = "cmd::set:decimation=" + to_string(decim_factor_log);
		ws.write( boost::asio::buffer(o_command.c_str(), o_command.size()) );
	}
	else if( regex_match(i_command, match, regex(R"_(set\:gain=([+-]?([0-9]*[.])?[0-9]+))_")) && match.size() > 1 )
	{
		double gain = stod(match[1]);
		GLOBALS::get().p_iq_source_->setOption("gain_double", &gain);
		string o_command = "cmd::set:gain=" + to_string(gain);
		ws.write( boost::asio::buffer(o_command.c_str(), o_command.size()) );
		GLOBALS::get().gain_ = gain;
	}
	else if( regex_match(i_command, match, regex(R"_(set\:lowpass_bw=([+-]?([0-9]*[.])?[0-9]+))_")) && match.size() > 1 )
	{
		cout<<"set lowpass_bw "<<stof(match[1])<<endl;
		GLOBALS::get().decoder_.lowpass_bw(stof(match[1]));
		string o_command = "cmd::set:lowpass_bw=" + to_string(GLOBALS::get().decoder_.lowpass_bw());
		cout<<"speak back "<<o_command<<endl;
		ws.write( boost::asio::buffer(o_command.c_str(), o_command.size()) );
	}
	else if( regex_match(i_command, match, regex(R"_(set\:lowpass_trans=([+-]?([0-9]*[.])?[0-9]+))_")) && match.size() > 1 )
	{
		GLOBALS::get().decoder_.lowpass_trans(stof(match[1]));
		string o_command = "cmd::set:lowpass_trans=" + to_string(GLOBALS::get().decoder_.lowpass_trans());
		ws.write( boost::asio::buffer(o_command.c_str(), o_command.size()) );
	}
	else if( regex_match(i_command, match, regex(R"_(set\:baud=([+-]?([0-9]*[.])?[0-9]+))_")) && match.size() > 1 )
	{
		GLOBALS::get().decoder_.baud( stof(match[1]) );
		string o_command = "cmd::set:baud=" + to_string(GLOBALS::get().decoder_.baud());
		ws.write( boost::asio::buffer(o_command.c_str(), o_command.size()) );
	}
	else if( regex_match(i_command, match, regex(R"_(set\:rtty_bits=(\d+))_")) && match.size() > 1 )
	{
		GLOBALS::get().decoder_.rtty_bits(stoi(match[1]));
		string o_command = "cmd::set:rtty_bits=" + to_string(GLOBALS::get().decoder_.rtty_bits());
		ws.write( boost::asio::buffer(o_command.c_str(), o_command.size()) );
	}
	else if( regex_match(i_command, match, regex(R"_(set\:rtty_stops=([+-]?([0-9]*[.])?[0-9]+))_")) && match.size() > 1 )
	{
		GLOBALS::get().decoder_.rtty_stops(stof(match[1]));
		string o_command = "cmd::set:rtty_stops=" + to_string(GLOBALS::get().decoder_.rtty_stops());
		ws.write( boost::asio::buffer(o_command.c_str(), o_command.size()) );
	}
	else if( regex_match(i_command, match, regex(R"_(set\:datasize=(\d+))_")) && match.size() > 1 )
	{
		int datasize = stoi(match[1]);
		if(datasize != 1 && datasize != 2 && datasize != 4)
			datasize = 1;

		if(datasize == 1)	GLOBALS::get().transport_data_type_ = GLOBALS::TransportDataType::kChar; // 8bit
		if(datasize == 2)	GLOBALS::get().transport_data_type_ = GLOBALS::TransportDataType::kShort;// 16 bit
		if(datasize == 4)	GLOBALS::get().transport_data_type_ = GLOBALS::TransportDataType::kFloat;// 32bit
		string o_command = "cmd::set:datasize=" + to_string(datasize);
		ws.write( boost::asio::buffer(o_command.c_str(), o_command.size()) );
	}
	else if( regex_match(i_command, match, regex(R"_(set\:biastee=([0-9])+)_")) && match.size() > 1 )
	{
		double value = stod(match[1]);
		GLOBALS::get().p_iq_source_->setOption("biastee_double", &value);
		GLOBALS::get().biast_ = value;
		string o_command = "cmd::set:biastee=" + to_string(value);
		ws.write( boost::asio::buffer(o_command.c_str(), o_command.size()) );
	}
	else if( regex_match(i_command, match, regex(R"_(set\:afc=([0-9])+)_")) && match.size() > 1 )
	{
		int value = stoi(match[1]);
		GLOBALS::get().afc_ = value;
		string o_command = "cmd::set:afc=" + to_string(value);
		ws.write( boost::asio::buffer(o_command.c_str(), o_command.size()) );
	}
	else
	{
		cout<<C_RED<<"Unknown command: "<<i_command<<C_OFF<<endl;
	}

	return true;

}


template<typename T>
void ShrinkVector(T& vec, size_t new_size)
{
	if(new_size >= vec.size())
		return;

	for(size_t i=0; i<new_size; ++i)
	{
		float i_0_1 = float(i) / new_size;
		size_t I = i_0_1 * vec.size();
		vec[i] = vec[I];
	}
	vec.resize(new_size);
}


// serializes spectrum info
size_t SpectrumToStream(std::stringstream& res_stream, float zoom, int resolution)
{
	using namespace std;

	auto spectrum_info = GLOBALS::get().decoder_.getSpectrumInfo();

	if(!spectrum_info.size())
		return 0;

	// zoom - select bins from center
	zoom = min(max(zoom, 0.01f), 0.99f);
	const size_t zoom_slice_begin = zoom/2 * spectrum_info.size();
	const size_t zoom_slice_end =   (1.0f-zoom/2) * spectrum_info.size();

	spectrum_info.erase( spectrum_info.begin() + zoom_slice_end, spectrum_info.end() );
	spectrum_info.erase( spectrum_info.begin(), spectrum_info.begin() + zoom_slice_begin);

	spectrum_info.peak_left_  -= zoom_slice_begin;
	if(spectrum_info.peak_left_ < 0 || spectrum_info.peak_left_ > spectrum_info.size())
	{
		spectrum_info.peak_left_ = 0;
		spectrum_info.peak_left_valid_ = false;
	}

	spectrum_info.peak_right_ -= zoom_slice_begin;
	if(spectrum_info.peak_right_ < 0 || spectrum_info.peak_right_ > spectrum_info.size())
	{
		spectrum_info.peak_right_ = 0;
		spectrum_info.peak_right_valid_ = false;
	}

	// transfer resolution - leave only 'resolution' number of bins
	//
	if(resolution < spectrum_info.size())
	{
		spectrum_info.peak_left_ = double(spectrum_info.peak_left_) * resolution / spectrum_info.size();
		spectrum_info.peak_right_ = double(spectrum_info.peak_right_) * resolution / spectrum_info.size();
		ShrinkVector(spectrum_info, resolution);
	}

	// choose transport bit resolution: 8, 16, 32 bit
	//
	if( GLOBALS::get().transport_data_type_ == GLOBALS::TransportDataType::kChar )
		SerializeSpectrum( spectrum_info, res_stream, (unsigned char*)0 );
	if( GLOBALS::get().transport_data_type_ == GLOBALS::TransportDataType::kShort )
		SerializeSpectrum( spectrum_info, res_stream, (unsigned short int*)0 );
	if( GLOBALS::get().transport_data_type_ == GLOBALS::TransportDataType::kFloat )
		SerializeSpectrum( spectrum_info, res_stream, (float*)0 );

	return spectrum_info.size();
}


// serialize demod
size_t DemodToStream(std::stringstream& res_stream, int resolution)
{
	std::vector<TDecoder::TValue> demod_acc;
	{
		std::lock_guard<std::mutex> _lock(GLOBALS::get().demod_accumulated_mtx_);
		demod_acc = GLOBALS::get().demod_accumulated_;
	}
	if(demod_acc.size())
	{
		ShrinkVector(demod_acc, resolution);
		// choose transport datatype: 8, 16, 32 bit
		if( GLOBALS::get().transport_data_type_ == GLOBALS::TransportDataType::kChar )
			SerializeDemodulation( demod_acc, res_stream, (unsigned char*)0 );
		if( GLOBALS::get().transport_data_type_ == GLOBALS::TransportDataType::kShort )
			SerializeDemodulation( demod_acc, res_stream, (unsigned short int*)0 );
		if( GLOBALS::get().transport_data_type_ == GLOBALS::TransportDataType::kFloat )
			SerializeDemodulation( demod_acc, res_stream, (float*)0 );
	}

	return demod_acc.size();
}

void DoSession(tcp::socket& i_socket)
{
	using namespace std;

	// allow just one session
	static std::atomic<bool> close_session{false};
	static std::atomic<bool> session_running{false};

	// close previous session
	while(session_running)
	{
		close_session = true;
		std::this_thread::sleep_for( ( std::chrono::duration<double, std::milli>(300) ));
	}

	try {
		websocket::stream<tcp::socket> ws{std::move(i_socket)};
		ws.accept();
		session_running = true;
		// this line does not work
		// ws.accept_ex([](websocket::response_type &m) { m.insert(beast::http::field::server, "habdec_server"); });

		ws.auto_fragment(false);

		while(!G_DO_EXIT)
		{
			if(close_session)
			{
				cout<<"Closing Session."<<endl;
				close_session = false;
				session_running = false;
				return;
			}

			beast::multi_buffer buffer;
			ws.read(buffer);

			string command = beast::buffers_to_string(buffer.data());
			buffer.consume(buffer.size());

			smatch match;

			// power,res=resolution_value,zoom=zoom_value
			if( regex_match(command, match, regex(R"_(cmd\:\:power\:res=(\d+),zoom=([+-]?([0-9]*[.])?[0-9]+))_")) && match.size() > 2 )
			{
				stringstream res_stream;
				res_stream<<"PWR_";
				if(SpectrumToStream( res_stream, stof(match[2]), stoi(match[1]) ))
				{
					ws.binary(true);
					ws.write( boost::asio::buffer(res_stream.str()) );
				}
			}
			// demod=resolution
			else if( regex_match(command, match, regex(R"_(cmd\:\:demod\:res=(\d+))_")) && match.size() > 1 )
			{
				stringstream res_stream;
				res_stream<<"DEM_";
				if( DemodToStream( res_stream, stoi(match[1]) ) )
				{
					ws.binary(true);
					ws.write( boost::asio::buffer(res_stream.str()) );
				}
			}
			// cmd::liveprint
			else if( regex_match(command, match, regex(R"_(cmd\:\:liveprint)_")) && match.size() > 0 )
			{
				string o_command = "cmd::info:liveprint=" + GLOBALS::get().decoder_.getRTTY();
				ws.text(true);
				ws.write( boost::asio::buffer(o_command.c_str(), o_command.size()) );
			}
			// cmd::****
			else if(command.size()>5 && command.substr(0,5) == "cmd::")
			{
				cout<<C_MAGENTA<<"Command "<<command<<C_OFF<<endl;
				ws.text(true);
				HandleCommand(command.substr(5), ws);
			}

			// check for new sentences
			vector<string> sentences;
			{
				lock_guard<mutex> _lock( GLOBALS::get().senteces_to_web_mtx_ );
				sentences = std::move(GLOBALS::get().senteces_to_web_);
			}
			for(auto& sentence : sentences)
			{
				string o_command = "cmd::info:sentence=" + sentence;
				ws.text(true);
				ws.write( boost::asio::buffer( o_command.c_str(), o_command.size()) );
			}

			// check for out commands
			vector<string> out_cmds;
			{
				lock_guard<mutex> _lock( GLOBALS::get().out_commands_mtx_ );
				out_cmds = std::move(GLOBALS::get().out_commands_);
			}
			for(auto& cmd : out_cmds)
			{
				ws.text(true);
				ws.write( boost::asio::buffer( cmd.c_str(), cmd.size()) );
			}

		}
	}
	catch(const boost::system::system_error& se) {
		if(se.code() != websocket::error::closed)
			cout << "Error: boost::system::system_error: " << se.code().message() << endl;
		else
			cout << "Session Closed. " << se.code().message() << endl;

	}
	catch(const exception& e)  {
		cout << "Session Error: " << e.what() << endl;
	}

	session_running = false;
	cout << "Session END."<<endl;
}


void RunCommandServer(const std::string command_host, const int command_port)
{
	using namespace std;
	using tcp = boost::asio::ip::tcp;

	if(!command_port || command_host == "")
	{
		cout<<C_RED<<"No Command host or port."<<C_OFF<<endl;
		return;
	}

	while(!G_DO_EXIT)
	{
		if (!GLOBALS::get().p_iq_source_)
			continue;

		try {
			auto const address = boost::asio::ip::make_address( command_host );
			auto const port = static_cast<unsigned short>( command_port );

			boost::asio::io_context ioc{1};
			tcp::acceptor acceptor{ioc, {address, port}};

			while(!G_DO_EXIT)
			{

				tcp::socket socket{ioc};
				acceptor.accept(socket); // Block until we get a connection
				cout<<C_MAGENTA<<"\nNew Client"<<C_OFF<<endl;

				std::thread{std::bind(&DoSession, std::move(socket))}.detach();
			}
		}
		catch(const exception& e) {
			cout<<C_RED<<"Failed starting Command Server\n"<<e.what()<<C_OFF<<endl;
			std::this_thread::sleep_for( ( std::chrono::duration<double, std::milli>(1000) ));
		}
	}
}