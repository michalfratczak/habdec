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
#include "habitat/habitat_interface.h"

extern bool G_DO_EXIT;


// i_param == "" --> sends all parameters
void EchoParameter(const std::string i_param, websocket::stream<tcp::socket>& ws)
{
	using namespace std;
	smatch match;
	ws.text(true);

	if(i_param == "frequency" || i_param == "")
	{
		double frequency = 0;
		GLOBALS::get().p_iq_source_->getOption("frequency_double", &frequency);
		frequency /= 1e6;
		string o_command = "cmd::set:frequency=" + to_string(frequency);
		ws.write( boost::asio::buffer(o_command.c_str(), o_command.size()) );
	}
	if(i_param == "ppm" || i_param == "")
	{
		double ppm = 0;
		GLOBALS::get().p_iq_source_->getOption("ppm_double", &ppm);
		string o_command = "cmd::set:ppm=" + to_string(ppm);
		ws.write( boost::asio::buffer(o_command.c_str(), o_command.size()) );
	}
	if(i_param == "gain" || i_param == "")
	{
		double gain = 0;
		GLOBALS::get().p_iq_source_->getOption("gain_double", &gain);
		string o_command = "cmd::set:gain=" + to_string(gain);
		ws.write( boost::asio::buffer(o_command.c_str(), o_command.size()) );
	}
	if(i_param == "baud" || i_param == "")
	{
		size_t baud = GLOBALS::get().decoder_.baud();
		string o_command = "cmd::set:baud=" + to_string(baud);
		ws.write( boost::asio::buffer(o_command.c_str(), o_command.size()) );
	}
	if(i_param == "rtty_bits" || i_param == "")
	{
		size_t rtty_bits = GLOBALS::get().decoder_.rtty_bits();
		string o_command = "cmd::set:rtty_bits=" + to_string(rtty_bits);
		ws.write( boost::asio::buffer(o_command.c_str(), o_command.size()) );
	}
	if(i_param == "rtty_stops" || i_param == "")
	{
		float rtty_stops = GLOBALS::get().decoder_.rtty_stops();
		string o_command = "cmd::set:rtty_stops=" + to_string(rtty_stops);
		ws.write( boost::asio::buffer(o_command.c_str(), o_command.size()) );
	}
	if(i_param == "lowpass_bw" || i_param == "")
	{
		string o_command = "cmd::set:lowpass_bw=" + to_string(GLOBALS::get().decoder_.lowpass_bw());
		ws.write( boost::asio::buffer(o_command.c_str(), o_command.size()) );
	}
	if(i_param == "lowpass_trans" || i_param == "")
	{
		string o_command = "cmd::set:lowpass_trans=" + to_string(GLOBALS::get().decoder_.lowpass_trans());
		ws.write( boost::asio::buffer(o_command.c_str(), o_command.size()) );
	}
	if(i_param == "biastee" || i_param == "")
	{
		double biastee = 0;
		GLOBALS::get().p_iq_source_->getOption("biastee_double", &biastee);
		string o_command = "cmd::set:biastee=" + to_string(biastee);
		ws.write( boost::asio::buffer(o_command.c_str(), o_command.size()) );
	}
	if(i_param == "afc" || i_param == "")
	{
		string o_command = "cmd::set:afc=" + to_string(GLOBALS::get().par_.afc_);
		ws.write( boost::asio::buffer(o_command.c_str(), o_command.size()) );
	}
	if(i_param == "decimation" || i_param == "")
	{
		int decim_factor_log = std::log2( GLOBALS::get().decoder_.getDecimationFactor() );
		string o_command = "cmd::set:decimation=" + to_string(decim_factor_log);
		ws.write( boost::asio::buffer(o_command.c_str(), o_command.size()) );
	}
	if(i_param == "sampling_rate" || i_param == "")
	{
		double sr = 0;
		GLOBALS::get().p_iq_source_->getOption("sampling_rate_double", &sr);
		string o_command = "cmd::info:sampling_rate=" + to_string(sr);
		ws.write( boost::asio::buffer(o_command.c_str(), o_command.size()) );
	}
	if(i_param == "dc_remove" || i_param == "")
	{
		double dc_rem = GLOBALS::get().decoder_.dc_remove();
		string o_command = "cmd::set:dc_remove=" + to_string(dc_rem);
		ws.write( boost::asio::buffer(o_command.c_str(), o_command.size()) );
	}
	if(i_param == "datasize" || i_param == "")
	{
		int datasize = 1;
		if( GLOBALS::get().par_.transport_data_type_ == GLOBALS::TransportDataType::kChar )
			datasize = 1;
		if( GLOBALS::get().par_.transport_data_type_ == GLOBALS::TransportDataType::kShort )
			datasize = 2;
		if( GLOBALS::get().par_.transport_data_type_ == GLOBALS::TransportDataType::kFloat )
			datasize = 4;

		string o_command = "cmd::set:datasize=" + to_string(datasize);
		ws.write( boost::asio::buffer(o_command.c_str(), o_command.size()) );
	}
}


bool HandleCommand(const std::string i_command, websocket::stream<tcp::socket>& ws)
{
	if(!GLOBALS::get().p_iq_source_)
		return false;

	using namespace std;
	smatch match;

	// GET
	if( i_command.size() > 4 && i_command.substr(0,4) == "get:" )
		EchoParameter( i_command.substr(4), ws );

	// SET
	else if( regex_match(i_command, match, regex(R"_(set\:frequency=([+-]?([0-9]*[.])?[0-9]+))_")) && match.size() > 1 )
	{
		double frequency = stod(match[1]);
		frequency *= 1e6;
		GLOBALS::get().p_iq_source_->setOption("frequency_double", &frequency);
		GLOBALS::get().par_.frequency_ = frequency;
		EchoParameter("frequency", ws);
	}
	else if( regex_match(i_command, match, regex(R"_(set\:decimation=(\d+))_")) && match.size() > 1 )
	{
		int decim_factor_log = stoi(match[1]);
		GLOBALS::get().decoder_.setupDecimationStagesFactor( pow(2,decim_factor_log) );
		GLOBALS::get().par_.decimation_ = decim_factor_log;
		EchoParameter("decimation", ws);
	}
	else if( regex_match(i_command, match, regex(R"_(set\:ppm=([+-]?([0-9]*[.])?[0-9]+))_")) && match.size() > 1 )
	{
		double ppm = stod(match[1]);
		GLOBALS::get().p_iq_source_->setOption("ppm_double", &ppm);
		GLOBALS::get().par_.ppm_ = ppm;
		EchoParameter("ppm", ws);
	}
	else if( regex_match(i_command, match, regex(R"_(set\:gain=([+-]?([0-9]*[.])?[0-9]+))_")) && match.size() > 1 )
	{
		double gain = stod(match[1]);
		GLOBALS::get().p_iq_source_->setOption("gain_double", &gain);
		GLOBALS::get().par_.gain_ = gain;
		EchoParameter("gain", ws);
	}
	else if( regex_match(i_command, match, regex(R"_(set\:lowpass_bw=([+-]?([0-9]*[.])?[0-9]+))_")) && match.size() > 1 )
	{
		float lpbw = stof(match[1]);
		GLOBALS::get().decoder_.lowpass_bw(lpbw);
		GLOBALS::get().par_.lowpass_bw_Hz_ = lpbw;
		EchoParameter("lowpass_bw", ws);
	}
	else if( regex_match(i_command, match, regex(R"_(set\:lowpass_trans=([+-]?([0-9]*[.])?[0-9]+))_")) && match.size() > 1 )
	{
		float lptr = stof(match[1]);
		GLOBALS::get().decoder_.lowpass_trans(lptr);
		GLOBALS::get().par_.lowpass_tr_ = lptr;
		EchoParameter("lowpass_trans", ws);
	}
	else if( regex_match(i_command, match, regex(R"_(set\:baud=([+-]?([0-9]*[.])?[0-9]+))_")) && match.size() > 1 )
	{
		float baud = stof(match[1]);
		GLOBALS::get().decoder_.baud( baud );
		GLOBALS::get().par_.baud_ = baud;
		EchoParameter("baud", ws);
	}
	else if( regex_match(i_command, match, regex(R"_(set\:rtty_bits=(\d+))_")) && match.size() > 1 )
	{
		int rtty_bits = stoi(match[1]);
		GLOBALS::get().decoder_.rtty_bits(rtty_bits);
		GLOBALS::get().par_.rtty_ascii_bits_ = rtty_bits;
		EchoParameter("rtty_bits", ws);
	}
	else if( regex_match(i_command, match, regex(R"_(set\:rtty_stops=([+-]?([0-9]*[.])?[0-9]+))_")) && match.size() > 1 )
	{
		int rtty_stops = stoi(match[1]);
		GLOBALS::get().decoder_.rtty_stops(rtty_stops);
		GLOBALS::get().par_.rtty_ascii_stops_ = rtty_stops;
		EchoParameter("rtty_stops", ws);
	}
	else if( regex_match(i_command, match, regex(R"_(set\:datasize=(\d+))_")) && match.size() > 1 )
	{
		int datasize = stoi(match[1]);

		// 3 is illegal value, skip to 2 or 4
		if(datasize == 3)
		{
			if(GLOBALS::get().par_.transport_data_type_ == GLOBALS::TransportDataType::kShort)
				datasize = 4;
			else
				datasize = 2;
		}

		if(datasize != 1 && datasize != 2 && datasize != 4)
			datasize = 1;

		if(datasize == 1)	GLOBALS::get().par_.transport_data_type_ = GLOBALS::TransportDataType::kChar; // 8bit
		if(datasize == 2)	GLOBALS::get().par_.transport_data_type_ = GLOBALS::TransportDataType::kShort;// 16 bit
		if(datasize == 4)	GLOBALS::get().par_.transport_data_type_ = GLOBALS::TransportDataType::kFloat;// 32bit
		EchoParameter("datasize", ws);
	}
	else if( regex_match(i_command, match, regex(R"_(set\:biastee=([0-9])+)_")) && match.size() > 1 )
	{
		double value = stod(match[1]);
		GLOBALS::get().p_iq_source_->setOption("biastee_double", &value);
		GLOBALS::get().par_.biast_ = value;
		EchoParameter("biastee", ws);
	}
	else if( regex_match(i_command, match, regex(R"_(set\:afc=([0-9])+)_")) && match.size() > 1 )
	{
		int value = stoi(match[1]);
		GLOBALS::get().par_.afc_ = value;
		EchoParameter("afc", ws);
	}
	else if( regex_match(i_command, match, regex(R"_(set\:dc_remove=([0-9])+)_")) && match.size() > 1 )
	{
		int value = stoi(match[1]);
		GLOBALS::get().decoder_.dc_remove(value);
		GLOBALS::get().par_.dc_remove_ = value;
		EchoParameter("dc_remove", ws);
	}
	else if( regex_match(i_command, match, regex(R"_(set\:payload=(\w+))_")) && match.size() > 1 )
	{
		using namespace habdec::habitat;

		string payload_id = match[1];
		std::map<std::string, HabitatFlight> flights;
		try {
			flights = ListFlights(0);
		}
		catch(const exception& e) {
			cout<<"Failed loading flights list: "<<e.what()<<endl;
			return false;
		}

		auto& DEC = GLOBALS::get().decoder_;

		for(auto& flight : flights)
		{
			for(auto& payload : flight.second.payloads_)
			{
				if( !payload_id.compare(payload.second.id_) )
				{
					GLOBALS::get().par_.baud_ = payload.second.baud_;
					GLOBALS::get().par_.rtty_ascii_bits_ = payload.second.ascii_bits_;
					GLOBALS::get().par_.rtty_ascii_stops_ = payload.second.ascii_stops_;
					GLOBALS::get().par_.frequency_ = payload.second.frequency_;

					DEC.baud( GLOBALS::get().par_.baud_ );
					DEC.rtty_bits( GLOBALS::get().par_.rtty_ascii_bits_ );
					DEC.rtty_stops( GLOBALS::get().par_.rtty_ascii_stops_ );
					GLOBALS::get().p_iq_source_->setOption("frequency_double", &GLOBALS::get().par_.frequency_);

					cout<<C_MAGENTA<<"Loading parameters for payload "<<payload_id<<C_OFF<<endl;
					cout<<"\tbaud: "<<GLOBALS::get().par_.baud_<<endl;
					cout<<"\tascii_bits: "<<GLOBALS::get().par_.rtty_ascii_bits_<<endl;
					cout<<"\tascii_stops: "<<GLOBALS::get().par_.rtty_ascii_stops_<<endl;
					cout<<"\tfrequency: "<<GLOBALS::get().par_.frequency_<<endl;

					break;
				}
			}
		}

		EchoParameter("frequency", ws);
		EchoParameter("baud", ws);
		EchoParameter("rtty_bits", ws);
		EchoParameter("rtty_stops", ws);
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
	if( GLOBALS::get().par_.transport_data_type_ == GLOBALS::TransportDataType::kChar )
		SerializeSpectrum( spectrum_info, res_stream, (unsigned char*)0 );
	if( GLOBALS::get().par_.transport_data_type_ == GLOBALS::TransportDataType::kShort )
		SerializeSpectrum( spectrum_info, res_stream, (unsigned short int*)0 );
	if( GLOBALS::get().par_.transport_data_type_ == GLOBALS::TransportDataType::kFloat )
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
		if( GLOBALS::get().par_.transport_data_type_ == GLOBALS::TransportDataType::kChar )
			SerializeDemodulation( demod_acc, res_stream, (unsigned char*)0 );
		if( GLOBALS::get().par_.transport_data_type_ == GLOBALS::TransportDataType::kShort )
			SerializeDemodulation( demod_acc, res_stream, (unsigned short int*)0 );
		if( GLOBALS::get().par_.transport_data_type_ == GLOBALS::TransportDataType::kFloat )
			SerializeDemodulation( demod_acc, res_stream, (float*)0 );
	}

	return demod_acc.size();
}

void WS_CLIENT_SESSION_THREAD(tcp::socket& i_socket)
{
	using namespace std;

	try {
		websocket::stream<tcp::socket> ws{std::move(i_socket)};
		ws.accept();
		// this line does not work
		// ws.accept_ex([](websocket::response_type &m) { m.insert(beast::http::field::server, "habdec_server"); });

		ws.auto_fragment(false);

		while(!G_DO_EXIT)
		{
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

			// give last sentence
			static thread_local int last_sentence_id = -1;
			auto& sen_map = GLOBALS::get().sentences_map_;
			if( sen_map.crbegin() != sen_map.crend() && sen_map.crbegin()->first != last_sentence_id )
			{
				string o_command = "cmd::info:sentence=" + sen_map.crbegin()->second;
				ws.text(true);
				ws.write( boost::asio::buffer( o_command.c_str(), o_command.size()) );
				last_sentence_id = sen_map.crbegin()->first;
			}

			// send parameters if they changed
			static thread_local typename 	GLOBALS::PARAMS params;
			if(params != GLOBALS::get().par_)
			{
				// cout<<"need to update parametes "<<std::this_thread::get_id()<<endl;
				EchoParameter("", ws);
				params = GLOBALS::get().par_;
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

				std::thread{std::bind(&WS_CLIENT_SESSION_THREAD, std::move(socket))}.detach();
			}
		}
		catch(const exception& e) {
			cout<<C_RED<<"Failed starting Command Server\n"<<e.what()<<C_OFF<<endl;
			std::this_thread::sleep_for( ( std::chrono::duration<double, std::milli>(1000) ));
		}
	}
}