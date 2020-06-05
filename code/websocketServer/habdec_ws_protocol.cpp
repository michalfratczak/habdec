
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

#include "habdec_ws_protocol.h"

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
#include "GLOBALS.h"
#include "NetTransport.h"
#include "habitat/habitat_interface.h"


// i_param == "" --> sends all parameters
std::vector<std::shared_ptr<HabdecMessage> > EchoParameter(const std::string i_param)
{
	using namespace std;

	std::vector<std::shared_ptr<HabdecMessage> >	result; // response messages

	smatch match;


	if(i_param == "frequency" || i_param == "")
	{
		double frequency = 0;
		GLOBALS::get().p_iq_source_->getOption("frequency_double", &frequency);
		frequency /= 1e6;

		auto p_msg = std::make_shared<HabdecMessage>();
		p_msg->to_all_clients_ = true;
		p_msg->data_stream_<<"cmd::set:frequency="<<frequency;
		result.push_back(p_msg);
	}

	if(i_param == "ppm" || i_param == "")
	{
		double ppm = 0;
		GLOBALS::get().p_iq_source_->getOption("ppm_double", &ppm);

		auto p_msg = std::make_shared<HabdecMessage>();
		p_msg->to_all_clients_ = true;
		p_msg->data_stream_<<"cmd::set:ppm="<<ppm;
		result.push_back(p_msg);
	}

	if(i_param == "gain" || i_param == "")
	{
		double gain = 0;
		GLOBALS::get().p_iq_source_->getOption("gain_double", &gain);

		auto p_msg = std::make_shared<HabdecMessage>();
		p_msg->to_all_clients_ = true;
		p_msg->data_stream_<<"cmd::set:gain="<<gain;
		result.push_back(p_msg);
	}

	if(i_param == "baud" || i_param == "")
	{
		size_t baud = GLOBALS::get().decoder_.baud();

		auto p_msg = std::make_shared<HabdecMessage>();
		p_msg->to_all_clients_ = true;
		p_msg->data_stream_<<"cmd::set:baud="<<baud;
		result.push_back(p_msg);
	}

	if(i_param == "rtty_bits" || i_param == "")
	{
		size_t rtty_bits = GLOBALS::get().decoder_.rtty_bits();

		auto p_msg = std::make_shared<HabdecMessage>();
		p_msg->to_all_clients_ = true;
		p_msg->data_stream_<<"cmd::set:rtty_bits="<<rtty_bits;
		result.push_back(p_msg);
	}

	if(i_param == "rtty_stops" || i_param == "")
	{
		float rtty_stops = GLOBALS::get().decoder_.rtty_stops();

		auto p_msg = std::make_shared<HabdecMessage>();
		p_msg->to_all_clients_ = true;
		p_msg->data_stream_<<"cmd::set:rtty_stops="<<rtty_stops;
		result.push_back(p_msg);
	}

	if(i_param == "lowpass_bw" || i_param == "")
	{
		auto p_msg = std::make_shared<HabdecMessage>();
		p_msg->to_all_clients_ = true;
		p_msg->data_stream_<<"cmd::set:lowpass_bw="<<GLOBALS::get().decoder_.lowpass_bw();
		result.push_back(p_msg);
	}

	if(i_param == "lowpass_trans" || i_param == "")
	{
		auto p_msg = std::make_shared<HabdecMessage>();
		p_msg->to_all_clients_ = true;
		p_msg->data_stream_<<"cmd::set:lowpass_trans="<<GLOBALS::get().decoder_.lowpass_trans();
		result.push_back(p_msg);
	}

	if(i_param == "biastee" || i_param == "")
	{
		double biastee = 0;
		GLOBALS::get().p_iq_source_->getOption("biastee_double", &biastee);

		auto p_msg = std::make_shared<HabdecMessage>();
		p_msg->to_all_clients_ = true;
		p_msg->data_stream_<<"cmd::set:biastee="<<biastee;
		result.push_back(p_msg);
	}

	if(i_param == "afc" || i_param == "")
	{
		auto p_msg = std::make_shared<HabdecMessage>();
		p_msg->to_all_clients_ = true;
		p_msg->data_stream_<<"cmd::set:afc="<<GLOBALS::get().par_.afc_;
		result.push_back(p_msg);
	}

	if(i_param == "decimation" || i_param == "")
	{
		int decim_factor_log = std::log2( GLOBALS::get().decoder_.getDecimationFactor() );

		auto p_msg = std::make_shared<HabdecMessage>();
		p_msg->to_all_clients_ = true;
		p_msg->data_stream_<<"cmd::set:decimation="<<decim_factor_log;
		result.push_back(p_msg);
	}

	if(i_param == "dc_remove" || i_param == "")
	{
		double dc_rem = GLOBALS::get().decoder_.dc_remove();

		auto p_msg = std::make_shared<HabdecMessage>();
		p_msg->to_all_clients_ = true;
		p_msg->data_stream_<<"cmd::set:dc_remove="<<dc_rem;
		result.push_back(p_msg);
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

		auto p_msg = std::make_shared<HabdecMessage>();
		p_msg->to_all_clients_ = true;
		p_msg->data_stream_<<"cmd::set:datasize="<<datasize;
		result.push_back(p_msg);
	}

	if(i_param == "sampling_rate" || i_param == "")
	{
		double sr = 0;
		GLOBALS::get().p_iq_source_->getOption("sampling_rate_double", &sr);

		auto p_msg = std::make_shared<HabdecMessage>();
		p_msg->to_all_clients_ = true;
		p_msg->data_stream_<<"cmd::info:sampling_rate="<<sr;
		result.push_back(p_msg);
	}

	return result;
}


std::vector< std::shared_ptr<HabdecMessage> >  HandleCommand(const std::string i_command)
{
	std::vector< std::shared_ptr<HabdecMessage> >  result; // response messages

	if(!GLOBALS::get().p_iq_source_)
		return result;

	using namespace std;

	smatch match;

	// GET:
	if( i_command.size() > 4 && i_command.substr(0,4) == "get:" )
	{
		result = EchoParameter( i_command.substr(4) );
	}
	// SET:
	else if( regex_match(i_command, match, regex(R"_(set\:frequency=([+-]?([0-9]*[.])?[0-9]+))_")) && match.size() > 1 )
	{
		double frequency = stod(match[1]);
		frequency *= 1e6;
		GLOBALS::get().p_iq_source_->setOption("frequency_double", &frequency);
		GLOBALS::get().par_.frequency_ = frequency;
		result = EchoParameter("frequency");
	}
	else if( regex_match(i_command, match, regex(R"_(set\:decimation=(\d+))_")) && match.size() > 1 )
	{
		int decim_factor_log = stoi(match[1]);
		GLOBALS::get().decoder_.setupDecimationStagesFactor( pow(2,decim_factor_log) );
		GLOBALS::get().par_.decimation_ = decim_factor_log;
		result = EchoParameter("decimation");
	}
	else if( regex_match(i_command, match, regex(R"_(set\:ppm=([+-]?([0-9]*[.])?[0-9]+))_")) && match.size() > 1 )
	{
		double ppm = stod(match[1]);
		GLOBALS::get().p_iq_source_->setOption("ppm_double", &ppm);
		GLOBALS::get().par_.ppm_ = ppm;
		result = EchoParameter("ppm");
	}
	else if( regex_match(i_command, match, regex(R"_(set\:gain=([+-]?([0-9]*[.])?[0-9]+))_")) && match.size() > 1 )
	{
		double gain = stod(match[1]);
		GLOBALS::get().p_iq_source_->setOption("gain_double", &gain);
		GLOBALS::get().par_.gain_ = gain;
		result = EchoParameter("gain");
	}
	else if( regex_match(i_command, match, regex(R"_(set\:lowpass_bw=([+-]?([0-9]*[.])?[0-9]+))_")) && match.size() > 1 )
	{
		float lpbw = stof(match[1]);
		GLOBALS::get().decoder_.lowpass_bw(lpbw);
		GLOBALS::get().par_.lowpass_bw_Hz_ = lpbw;
		result = EchoParameter("lowpass_bw");
	}
	else if( regex_match(i_command, match, regex(R"_(set\:lowpass_trans=([+-]?([0-9]*[.])?[0-9]+))_")) && match.size() > 1 )
	{
		float lptr = stof(match[1]);
		GLOBALS::get().decoder_.lowpass_trans(lptr);
		GLOBALS::get().par_.lowpass_tr_ = lptr;
		result = EchoParameter("lowpass_trans");
	}
	else if( regex_match(i_command, match, regex(R"_(set\:baud=([+-]?([0-9]*[.])?[0-9]+))_")) && match.size() > 1 )
	{
		float baud = stof(match[1]);
		GLOBALS::get().decoder_.baud( baud );
		GLOBALS::get().par_.baud_ = baud;
		result = EchoParameter("baud");
	}
	else if( regex_match(i_command, match, regex(R"_(set\:rtty_bits=(\d+))_")) && match.size() > 1 )
	{
		int rtty_bits = stoi(match[1]);
		GLOBALS::get().decoder_.rtty_bits(rtty_bits);
		GLOBALS::get().par_.rtty_ascii_bits_ = rtty_bits;
		result = EchoParameter("rtty_bits");
	}
	else if( regex_match(i_command, match, regex(R"_(set\:rtty_stops=([+-]?([0-9]*[.])?[0-9]+))_")) && match.size() > 1 )
	{
		int rtty_stops = stoi(match[1]);
		GLOBALS::get().decoder_.rtty_stops(rtty_stops);
		GLOBALS::get().par_.rtty_ascii_stops_ = rtty_stops;
		result = EchoParameter("rtty_stops");
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
		result = EchoParameter("datasize");
	}
	else if( regex_match(i_command, match, regex(R"_(set\:biastee=([0-9])+)_")) && match.size() > 1 )
	{
		double value = stod(match[1]);
		GLOBALS::get().p_iq_source_->setOption("biastee_double", &value);
		GLOBALS::get().par_.biast_ = value;
		result = EchoParameter("biastee");
	}
	else if( regex_match(i_command, match, regex(R"_(set\:afc=([0-9])+)_")) && match.size() > 1 )
	{
		int value = stoi(match[1]);
		GLOBALS::get().par_.afc_ = value;
		result = EchoParameter("afc");
	}
	else if( regex_match(i_command, match, regex(R"_(set\:dc_remove=([0-9])+)_")) && match.size() > 1 )
	{
		int value = stoi(match[1]);
		GLOBALS::get().decoder_.dc_remove(value);
		GLOBALS::get().par_.dc_remove_ = value;
		result = EchoParameter("dc_remove");
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
			return result;
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
					GLOBALS::get().par_.coord_format_lat_ = payload.second.coord_format_lat_;
					GLOBALS::get().par_.coord_format_lon_ = payload.second.coord_format_lon_;

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

		auto res_fr = EchoParameter("frequency");
		auto res_bd = EchoParameter("baud");
		auto res_rb = EchoParameter("rtty_bits");
		auto res_rs = EchoParameter("rtty_stops");
		result.insert( result.end(), res_fr.begin(), res_fr.end() );
		result.insert( result.end(), res_bd.begin(), res_bd.end() );
		result.insert( result.end(), res_rb.begin(), res_rb.end() );
		result.insert( result.end(), res_rs.begin(), res_rs.end() );
	}
	else
	{
		cout<<C_RED<<"Unknown command: "<<i_command<<C_OFF<<endl;
	}

	return result;

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


// serialize demodulation
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

std::vector< std::shared_ptr<HabdecMessage> >  HandleRequest(std::string command)
{
	using namespace std;

	// cout<<"HandleRequest: "<<command<<endl;

	std::vector< std::shared_ptr<HabdecMessage> >  result; // response messages
	// result.emplace_back( std::make_shared<HabdecMessage>() );
	// auto& msg = *result[0];

	smatch match;

	// power:res=resolution_value,zoom=zoom_value
	if( regex_match(command, match, regex(R"_(cmd\:\:power\:res=(\d+),zoom=([+-]?([0-9]*[.])?[0-9]+))_")) && match.size() > 2 )
	{
		auto p_msg = std::make_shared<HabdecMessage>();
		p_msg->is_binary_ = true;
		p_msg->data_stream_<<"PWR_";
		if(SpectrumToStream( p_msg->data_stream_, stof(match[2]), stoi(match[1]) ))
			result.push_back(p_msg);
	}
	// demod:res=resolution_value
	else if( regex_match(command, match, regex(R"_(cmd\:\:demod\:res=(\d+))_")) && match.size() > 1 )
	{
		auto p_msg = std::make_shared<HabdecMessage>();
		p_msg->is_binary_ = true;
		p_msg->data_stream_<<"DEM_";
		if( DemodToStream( p_msg->data_stream_, stoi(match[1]) ) )
			result.push_back(p_msg);
	}
	// cmd::sentence
	else if( regex_match(command, match, regex(R"_(cmd\:\:sentence)_")) && match.size() > 0 )
	{
		auto& sen_map = GLOBALS::get().sentences_map_;
		string sentence("");
		if( sen_map.crbegin() != sen_map.crend() )
			sentence = sen_map.crbegin()->second;

		auto p_msg = std::make_shared<HabdecMessage>();
		p_msg->data_stream_<<"cmd::info:sentence="<<sentence;
		result.push_back(p_msg);
	}
	// cmd::liveprint
	else if( regex_match(command, match, regex(R"_(cmd\:\:liveprint)_")) && match.size() > 0 )
	{
		auto p_msg = std::make_shared<HabdecMessage>();
		p_msg->data_stream_<<"cmd::info:liveprint="<<GLOBALS::get().decoder_.getRTTY();
		result.push_back(p_msg);
	}
	// cmd::stats
	else if( regex_match(command, match, regex(R"_(cmd\:\:stats)_")) && match.size() > 0 )
	{
		auto& stats = GLOBALS::get().stats_;

		auto p_msg = std::make_shared<HabdecMessage>();
		p_msg->data_stream_<<"cmd::info:stats=";
		p_msg->data_stream_<<"ok:"<<stats.num_ok_;
		p_msg->data_stream_<<",dist_line:"<<stats.D_.dist_line_;
		p_msg->data_stream_<<",dist_circ:"<<stats.D_.dist_circle_;
		p_msg->data_stream_<<",max_dist:"<<stats.dist_max_;
		p_msg->data_stream_<<",min_elev:"<<stats.elev_min_;
		p_msg->data_stream_<<",lat:"<<GLOBALS::get().par_.station_lat_;
		p_msg->data_stream_<<",lon:"<<GLOBALS::get().par_.station_lon_;
		p_msg->data_stream_<<",alt:"<<GLOBALS::get().par_.station_alt_;

		const auto last_sentence_age = std::chrono::duration_cast<std::chrono::seconds>(
			std::chrono::steady_clock::now() - stats.last_sentence_timestamp_).count();
		p_msg->data_stream_<<",age:"<<int(round(last_sentence_age));

		result.push_back(p_msg);
	}
	// cmd::****
	else if(command.size()>5 && command.substr(0,5) == "cmd::")
	{
		cout<<C_MAGENTA<<"Command "<<command<<C_OFF<<endl;
		auto responses = HandleCommand(command.substr(5));
		result.insert( result.end(), responses.begin(), responses.end() );
	}


	// send parameters if they changed
	static thread_local typename 	GLOBALS::PARAMS params;
	if(params != GLOBALS::get().par_)
	{
		auto responses = EchoParameter( "" );
		result.insert( result.end(), responses.begin(), responses.end() );

		params = GLOBALS::get().par_;
	}

	return result;

}

